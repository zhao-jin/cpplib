#!/usr/bin/env python
#
# Copyright 2011 Tencent Inc.
#
# Authors: simonwang <simonwang@tencent.com>
#
# PreSubmit client script, it generates the diff from directories under
# BLADE_ROOT, and upload it to the presubmit build system to patch and build

import sys
import os
import cookielib
import optparse
import signal
import traceback
import subprocess
import logging
import platform
import tempfile
import urllib
import urllib2
import tarfile
import shutil
import time
import getpass
import mimetypes
import socket
import base64
import json

# The logging verbosity:
#  0: Errors only.
#  1: Status messages.
#  2: Info logs.
#  3: Debug logs.
verbosity = 0

AUTH_ACCOUNT_TYPE = "TENCENT"

DEFAULT_PRESUBMIT_PORT = "8089"

DEFAULT_CI_SERVER_PREFIX = "http://10.6.209.184/job/"
DEFAULT_CI_SERVER_POSTFIX = "/1/api/python"

DEFAULT_CODEREVIEW_SERVER = "http://codereview.oa.com/"

VCS_SUBVERSION = "Subversion"
VCS_UNKNOWN = "Unknown"


def _get_email(prompt):
    """Prompts the user for their email address and returns it.

    The last used email address is saved to a file and offered up as a suggestion
    to the user.

    """
    last_email_file_name = os.path.expanduser("~/.last_presubmit_email_address")
    last_email = ""
    if os.path.exists(last_email_file_name):
        try:
            last_email_file = open(last_email_file_name, "r")
            last_email = last_email_file.readline().strip("\n")
            last_email_file.close()
            prompt += " [%s]" % last_email
        except IOError, e:
            pass
    email = raw_input(prompt + ": ").strip()
    if email:
        try:
            last_email_file = open(last_email_file_name, "w")
            last_email_file.write(email)
            last_email_file.close()
        except IOError, e:
            pass
    else:
        email = last_email
    return email


def _get_user_credentials(email):
    if email is None:
        _error_exit("You should specify email.")
    local_email = email.split('@')[0]
    if local_email is None:
        local_email = _get_email("Email (used for svn ci): ")
    password = getpass.getpass("Password for %s: " % local_email)
    return (local_email, password)


def _get_editor():
    editor = os.environ.get('SVN_EDITOR')
    if not editor:
        editor = os.environ.get('EDITOR')
    if not editor:
        editor = 'vim'
    return editor


def _run_editor():
    file_handle, filename = tempfile.mkstemp(text=True)
    try:
        cmd = '%s %s' % (_get_editor(), filename)
        try:
            proc = subprocess.Popen(cmd, shell=True)
            proc.wait()
        except Exception:
            pass
        return _file_read(filename)
    finally:
        os.remove(filename)


def run_shell_with_return_code(command, print_output=False,
                               universal_newlines=True,
                               env=os.environ):
    logging.info("Running %s", command)
    env["LC_MESSAGES"] = "C"
    p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         shell=False, universal_newlines=universal_newlines,
                         env=env)
    if print_output:
        output_array = []
        while True:
            line = p.stdout.readline()
            if not line:
                break
            print line.strip("\n")
            output_array.append(line)
        output = "".join(output_array)
    else:
        output = p.stdout.read()
    p.wait()
    errout = p.stderr.read()
    if print_output and errout:
        print >>sys.stderr, errout
    p.stdout.close()
    p.stderr.close()
    return output, p.returncode


def run_shell(command, silent_ok=False, universal_newlines=True,
              print_output=False, env=os.environ):
    data, retcode = run_shell_with_return_code(command, print_output,
                                               universal_newlines, env)
    if retcode:
        _error_exit("Get error status from %s:\n%s" % (command, data))
    if not silent_ok and not data:
        _error_exit("No output from %s" % command)
    return data


def _error_exit(msg, code = 1):
    msg = "PreSubmit(error): " + msg
    print >>sys.stderr, msg
    sys.exit(code)


def _warning(msg):
    msg = "PreSubmit(warning): " + msg
    print >>sys.stderr, msg

def find_blade_root():
    current_dir = os.getcwd()
    while current_dir != '/':
        if not os.path.isfile(os.path.join(current_dir, "BLADE_ROOT")):
            current_dir = os.path.dirname(current_dir)
        else:
            break
    return current_dir

def check_blade_root():
    blade_root_dir = find_blade_root()
    if blade_root_dir == '/':
        _error_exit("Please run presubmit.py under BLADE_ROOT directory.")
    else:
        print blade_root_dir

class AbstractRpcServer(object):
  """Provides a common interface for a simple RPC server."""

  def __init__(self, host, auth_function, host_override=None, extra_headers={},
               save_cookies=False, account_type=AUTH_ACCOUNT_TYPE):
    """Creates a new HttpRpcServer.

    Args:
      host: The host to send requests to.
      auth_function: A function that takes no arguments and returns an
        (email, password) tuple when called. Will be called if authentication
        is required.
      host_override: The host header to send to the server (defaults to host).
      extra_headers: A dict of extra headers to append to every request.
      save_cookies: If True, save the authentication cookies to local disk.
        If False, use an in-memory cookiejar instead.  Subclasses must
        implement this functionality.  Defaults to False.
      account_type: Account type used for authentication. Defaults to
        AUTH_ACCOUNT_TYPE.
    """
    self.host = host
    self.username = ""
    self.password = ""
    self.cookie_jar = ""
    if (not self.host.startswith("http://") and
        not self.host.startswith("https://")):
      self.host = "http://" + self.host
    self.host_override = host_override
    self.auth_function = auth_function
    self.authenticated = False
    self.extra_headers = extra_headers
    self.save_cookies = save_cookies
    self.account_type = account_type
    self.opener = self._get_opener()
    if self.host_override:
      logging.info("Server: %s; Host: %s", self.host, self.host_override)
    else:
      logging.info("Server: %s", self.host)

  def _get_opener(self):
    """Returns an OpenerDirector for making HTTP requests.

    Returns:
      A urllib2.OpenerDirector object.
    """
    raise NotImplementedError()

  def _create_request(self, url, data=None):
    """Creates a new urllib request."""
    # logging.debug("Creating request for: '%s' with payload:\n%s", url, data)
    req = urllib2.Request(url, data=data)
    if self.host_override:
      req.add_header("Host", self.host_override)
    for key, value in self.extra_headers.iteritems():
      req.add_header(key, value)
    return req

  def _get_auth_token(self, email, password):
    """Uses ClientLogin to authenticate the user, returning an auth token.

    Args:
      email:    The user's email address
      password: The user's password

    Raises:
      ClientLoginError: If there was an error authenticating with ClientLogin.
      HTTPError: If there was some other form of HTTP error.

    Returns:
      The authentication token returned by ClientLogin.
    """
    raise NotImplementedError("child class should implemented it.")
    # account_type = self.account_type

  def _authenticate(self,err):
    """Authenticates the user, Not Implemented now."""
    return

  def send(self, request_path, payload=None,
           content_type="application/octet-stream",
           timeout=None,
           extra_headers=None,
           **kwargs):
    """sends an RPC and returns the response.

    Args:
      request_path: The path to send the request to, eg /api/appversion/create.
      payload: The body of the request, or None to send an empty request.
      content_type: The Content-Type header to use.
      timeout: timeout in seconds; default None i.e. no timeout.
        (Note: for large requests on OS X, the timeout doesn't work right.)
      extra_headers: Dict containing additional HTTP headers that should be
        included in the request (string header names mapped to their values),
        or None to not include any additional headers.
      kwargs: Any keyword arguments are converted into query string parameters.

    Returns:
      The response body, as a string.
    """
    # TODO: Don't require authentication.  Let the server say
    # whether it is necessary.
    if not self.authenticated:
        self._authenticate()


    old_timeout = socket.getdefaulttimeout()
    socket.setdefaulttimeout(timeout)
    try:
        tries = 0
        while True:
            tries += 1
            args = dict(kwargs)
            url = "%s%s" % (self.host, request_path)
            if args:
                url += "?" + urllib.urlencode(args)
            req = self._create_request(url=url, data=payload)
            req.add_header("Content-Type", content_type)
            if extra_headers:
                for header, value in extra_headers.items():
                    req.add_header(header, value)
            try:
                f = self.opener.open(req)
                response = f.read()
                f.close()
                return response
            except urllib2.HTTPError, e:
                if tries > 3:
                    raise
                elif e.code == 401 or e.code == 302:
                    self._authenticate()
                elif e.code == 301:
                    # Handle permanent redirect manually.
                    url = e.info()["location"]
                    url_loc = urlparse.urlparse(url)
                    self.host = '%s://%s' % (url_loc[0], url_loc[1])
                elif e.code == 403:
                    response = e.read()
                    return response
    finally:
        socket.setdefaulttimeout(old_timeout)


class ClientLoginError(urllib2.HTTPError):
    """Login Error Class."""
    def __init__(self, url, code, msg, headers, args):
        urllib2.HTTPError.__init__(self, url, code, msg, headers, None)
        self.args = args
        self.reason = args["Error"]


class HttpRpcServer(AbstractRpcServer):
  """Provides a simplified RPC-style interface for HTTP requests."""

  def _authenticate(self, login_url="/login"):
    """Save the cookie jar after authentication."""
#    super(HttpRpcServer, self)._authenticate()
    login_url = "%s%s" % (self.host, login_url)
    print "Login URL: %r" % login_url
    self.username = options.email.split('@')[0] #raw_input("Username: ")
    self.password = getpass.getpass("Password: ")
    req = self._create_request(
        url=login_url,
        data=urllib.urlencode({
            "username": self.username,
            "password": self.password,
        })
    )
    try:
        response = self.opener.open(req)
        response_body = response.read()
        print "auth response:", response_body
        if not response_body or response_body == "auth error":
            self.authenticated = False
        else:
            if self.save_cookies:
                print "save cookies"
                _file_write(self.cookie_file, response_body)
            self.authenticated = True
            self.cookie_jar = response_body
    except urllib2.HTTPError, e:
        if e.code == 302:
            self.authenticated = False
        elif e.code == 403:
            self.authenticated = False
        else:
            self.authenticated = False

  def _get_opener(self):
    """Returns an OpenerDirector that supports cookies and ignores redirects.

    Returns:
      A urllib2.OpenerDirector object.
    """
    opener = urllib2.OpenerDirector()
    opener.add_handler(urllib2.ProxyHandler())
    opener.add_handler(urllib2.UnknownHandler())
    opener.add_handler(urllib2.HTTPHandler())
    opener.add_handler(urllib2.HTTPDefaultErrorHandler())
    opener.add_handler(urllib2.HTTPSHandler())
    opener.add_handler(urllib2.HTTPErrorProcessor())
    if self.save_cookies:
      self.cookie_file = os.path.expanduser("~/.presubmit_upload_cookies")
      if os.path.exists(self.cookie_file):
        try:
          self.cookie_jar = _file_read(self.cookie_file)
          self.authenticated = True
          _warning("Loaded authentication cookies from " + self.cookie_file)
        except IOError, e:
          # Failed to load cookies - just ignore them.
          pass
      else:
          self.authenticated = False
    else:
      # Don't save cookies across runs of update.py.
      self.cookie_jar = None
    # opener.add_handler(urllib2.HTTPCookieProcessor(self.cookie_jar))
    return opener

class CodeReviewIssue(object):
    def __init__(self, codereview_server, user, svn_remote_url, issue):
        self.user = user
        self.svn_remote_url = svn_remote_url
        self.issue = issue
        self.issue_info_url = "%s%s/info" % (codereview_server, issue)
        self.issue_url = "%s%s" % (codereview_server, issue)
        try:
            self.issue_info = json.load(urllib.urlopen(self.issue_info_url))
        except:
            _error_exit("Invalid issue or server down")

    def get_reviewed_files(self):
        return self.issue_info["filenames"]

    def get_subject(self):
        return self.issue_info["subject"]

    def check_has_unreviewed_changes(self, local_changes):
        local_changed_files = []
        for line in local_changes:
            if line.startswith('M'):
                filename = line.split()[1]
                local_changed_files.append(filename)
        reviewed_changes = self.issue_info["filenames"]
        unreviewed_changes = set(local_changed_files) - set(reviewed_changes)
        if len(unreviewed_changes) != 0:
            prompt = ('The following files has not been reviewed :\n%s\nContinue (yes/no)? ' %
                      '\n'.join(unreviewed_changes))
            answer = raw_input(prompt).strip()
            if answer != 'yes':
                _error_exit('Exit')

    def check_issue(self):
        if self.user != self.issue_info["owner"] and self.user not in self.issue_info["approvers"]:
            _error_exit("You are not the owner or approver of this issue")

        if not self.issue_info['approvers']:
            _error_exit("Not approved.")

        issue_base = self.issue_info["base_url"]
        if not self._is_same_url(self.svn_remote_url, issue_base):
            _error_exit("Different path:\nlocal is %s\nissue is %s" %
                    (self.svn_remote_url, issue_base))

    def _is_same_url(self, url1, url2):
        if url1 == url2:
            return True
        if len(url1) == len(url2) + 1:
            return url1[0:-1] == url2 and url1[-1:] == '/'
        if len(url1) + 1 == len(url2):
            return url1 == url2[0:-1] and url2[-1:] == '/'
        return False


class CmdOptions(object):
    """Command Line Option Parser."""
    def __init__(self):
        (self.options, self.args) = self.__cmd_parse()

    def __cmd_parse(self):
        cmd_parser = optparse.OptionParser("%prog [options]")
        cmd_parser.add_option("-y", "--assume_yes", action="store_true",
                              dest="assume_yes", default=False,
                              help="Assume that the answer to yes/no question is 'yes'.")
        cmd_parser.add_option("-t", "--test_server", action="store_true",
                              dest="test_server", default=False,
                              help="Just test presubmit server, not submit to svn")

        # Logging
        group = cmd_parser.add_option_group("Logging options")
        group.add_option("-q", "--quiet", action="store_const", const=0,
                         dest="verbose", help="Print errors only.")
        group.add_option("-v", "--verbose", action="store_const", const=2,
                         dest="verbose", default=1,
                         help="Print info level logs (default).")

        # PreSubmit server
        group = cmd_parser.add_option_group("PreSubmit server options")
        group.add_option("-j", "--jobname", action="store", dest="jobname",
                         metavar="JOBNAME",
                         help=("The jobname you want to upload to."))
        group.add_option("-e", "--email", action="store", dest="email",
                         metavar="EMAIL", default=None,
                         help="The username to use. will prompt is omitted.")
        group.add_option("-s", "--server_addr", action="store", dest="addr",
                         metavar="SERVER_ADDRESS",
                         help=("The presubmit server address(specify address"
                               " if can not get it from CI with jobname)."))

        # Issue
        group = cmd_parser.add_option_group("Patch options");
        group.add_option("-m", "--message", action="store", dest="message",
                         metavar="MESSAGE", default=None,
                         help="A message to identify the patch. "
                              "Will prompt if omitted.")
        group.add_option("-i", "--issue", action="store", dest="issue",
                        help="Code Review Issue Number")

        return cmd_parser.parse_args()

    def get_options(self):
        return self.options

    def get_args(self):
        return self.args


class VersionControlSystem(object):
    """Abstract interface of Version Control System."""
    def __init__(self, options):
        """Constructor.

        Args:
          options: Command line options
        """
        self.options = options

    def post_process_diff(self, diff):
        """Return the diff with any special post processing this VCS needs."""
        return diff

    def generate_diff(self, args):
        """Returns the current diff as a string."""
        raise NotImplementedError(
                "abstract method -- subclass %s must override" % self.__class__)

    def get_unknown_files(self):
        """Returns the unknown file list."""
        raise NotImplementedError(
                "abstract method -- subclass %s must override" % self.__class__)

    def get_conflict_files(self):
        """Returns the conflict file list."""
        raise NotImplementedError(
                "abstract method -- subclass %s must override" % self.__class__)

    def get_changed_files(self):
        """Returns the changed file list."""
        raise NotImplementedError(
                "abstract method -- subclass %s must override" % self.__class__)

    def check_up_to_date(self):
        """Returns whether it is up to date."""
        raise NotImplementedError(
                "abstract method -- subclass %s must override" % self.__class__)

    def check_for_unknown_files(self):
        unknown_files = self.get_unknown_files()
        if unknown_files:
            print "The following files are not added to version control:"
            for line in unknown_files:
                print line
            prompt = "Are you sure to continue?(y/N) "
            answer = raw_input(prompt).strip()
            if answer != "y":
                _error_exit("user aborted")

    def check_for_conflict_files(self):
        """Check conflict files in dir."""
        conflict_files = self.get_conflict_files()
        if conflict_files:
            print "The following files are in conflict state:"
            for line in conflict_files:
                print line
            prompt = "Are you sure to continue?(y/N) "
            answer = raw_input(prompt).strip()
            if answer != "y":
                _error_exit("user aborted")


class SubversionVCS(VersionControlSystem):
    """Implementation of the VersionControlSystem interface."""
    def __init__(self, options):
        super(SubversionVCS, self).__init__(options)
        self.svn_base = self._guess_base()
        self.svn_revision = self._guess_revision()
        self.svn_remote_revision = self._guess_revision(self.svn_base)

    def guess_base(self):
        return self.svn_base

    def guess_revision(self):
        return self.svn_revision

    def _svn_status(self, svn_path):
        info = run_shell(["svn", "info", svn_path])
        return info

    def _guess_base(self):
        info = run_shell(["svn", "info"])
        for line in info.splitlines():
            words = line.split()
            if len(words) == 2 and words[0] == "URL:":
                url = words[1]
                return url
        _error_exit("Can't find URL in output from svn info")

    def _guess_revision(self, svn_path=""):
        info = self._svn_status(svn_path)
        for line in info.splitlines():
            if line.startswith('Last Changed Rev:'):
                words = line.split();
                return words[3]
        _error_exit("Can't find Revision in output from svn info")

    def check_up_to_date(self):
        return self.svn_revision == self.svn_remote_revision

    def generate_diff(self, args):
        """Generate diffs as a string."""
        cmd = ["svn", "diff", "--diff-cmd=diff"]
        cmd.extend(args)
        data = run_shell(cmd, True)
        if not data:
            _warning("no diff")
            return None
        _warning("has diff " + os.getcwd())
        count = 0
        for line in data.splitlines():
            if line.startswith("Index:") or line.startswith("Property changes on:"):
                count += 1
                logging.info(line)
        if not count:
            _error_exit("No valid patches found in output from svn diff")
        return data

    def get_unknown_files(self):
        print "check unknown files"
        status = run_shell(["svn", "status", "--ignore-externals"], silent_ok=True)
        unknown_files = []
        for line in status.split("\n"):
            if line and line[0] == "?":
                unknown_files.append(line)
        return unknown_files

    def get_conflict_files(self):
        status = run_shell(["svn", "status", "-q", "--ignore-externals"], silent_ok=True)
        conflict_files = []
        for line in status.split("\n"):
            if line and line[0] == "C":
                conflict_files.append(line)
        return conflict_files

    def get_changed_files(self):
        status = run_shell(["svn", "status", "-q", "--ignore-externals"], silent_ok=True)
        changed_files = []
        for line in status.split("\n"):
            if line and line[0] == "M":
                changed_files.append(line)
        return changed_files

    def get_status(self, filename):
        status = run_shell(["svn", "status", "--ignore_externals", self._escape_path(filename)], silent_ok=True)
        if not status:
            _error_exit("svn status returned no output for %s" % filename)
        status_lines = status.splitlines()
        if (len(status_lines) == 3 and
            not status_lines[0] and
            status_lines[1].startswith("--- Changelist")):
            status = status_lines[2]
        else:
            status = status_lines[0]
        return status

    def check_password(self, username, password):
        print "check_password"


def _get_rpc_server(server, email=None, password=None,
                    host_override=None,
                    save_cookies=True):
    rpc_server_class = HttpRpcServer

    def __get_user_credentials():
        local_email = email.split('@')[0]
        if local_email is None:
            local_email = _get_email("Email (login %s)" % server)
        password = getpass.getpass("Password for %s: " % local_email)
        return (local_email, password)

    return rpc_server_class(server,
                            __get_user_credentials,
                            host_override=host_override,
                            save_cookies=save_cookies)


def _file_read(filename, mode='rU'):
    content = None
    f = open(filename, mode)
    try:
        content = f.read()
    finally:
        f.close()
    return content


def _file_write(filename, content, mode='w'):
    f = open(filename, mode)
    try:
        f.write(content)
    finally:
        f.close()


def _real_path(path):
    """Get the real path if path is symbol link."""
    if os.path.islink(path):
        return os.readlink(path)
    else:
        return path

def _is_valid_svn_dir():
    output, ret = run_shell_with_return_code(['svn', 'list', '.'])
    return (ret is 0)

def _guess_vcs(options):
    if _is_valid_svn_dir():
        return SubversionVCS(options)
    else:
        _warning('%s is not a valid subversion directory.' % os.getcwd())
        return None


def _process_working_dir(files, working_dir, options, args):
    if working_dir == find_blade_root():
        for fn in os.listdir(working_dir):
            if os.path.isdir(fn):
                old_current_source_dir = working_dir
                working_dir = os.path.join(working_dir, fn)
                os.chdir(working_dir)
                _process_svn_dir(files, working_dir, options, args)
                os.chdir(old_current_source_dir)
                working_dir = old_current_source_dir
            else:
                pass
    else:
        _process_svn_dir(files, working_dir, options, args)
    if len(files) == 0:
        _error_exit("There is no patch generated, please check your current working dir, " + working_dir)
    if len(files) > 1:
        prompt = "There are more than 1 patch generated, are you sure(y/N): "
        answer = raw_input(prompt).strip()
        if answer != 'y':
            _error_exit("user aborted.")


def _process_svn_dir(files, working_dir, options, args):
    """Process the child directories under BLADE_ROOT, generate svn diff separately."""
    vcs = _guess_vcs(options)
    if isinstance(vcs, SubversionVCS):
        # if it is not up to date, exit
        if vcs.check_up_to_date() is False:
            _error_exit(working_dir + " is not up to date, please check it manually.")
        else:
            # vcs.check_for_unknown_files()
            if options.issue:
                issue_checker = CodeReviewIssue(DEFAULT_CODEREVIEW_SERVER, options.email, vcs.guess_base(), options.issue)
                issue_checker.check_has_unreviewed_changes(vcs.get_changed_files())
                issue_checker.check_issue()
                options.message = issue_checker.get_subject()
            data = vcs.generate_diff(args)
            if not data is None:
                dirpath = working_dir.strip().replace('\\', '/')
                dirname = os.path.basename(working_dir)
                patch_info = [dirname, vcs.guess_base(), vcs.guess_revision(), data]
                files[dirpath] = patch_info
            else:
                pass


def _make_tar(folder_to_backup, dest_folder, jobname, compression='gz'):
    """Tar and compress the folder_to_backup to a file under dest_folder path

    Args:
      folder_to_backup: the directory you want to tar and compress
      dest_folder:      the parent path where the output file will exist
      jobname:          the current presubmit jobname, will be the name prefix of the tar file
      compression:      the compress file's ext, default is gz, it can also be bz2
    """
    if compression:
        dest_ext = '.' + compression
    else:
        dest_ext = ''
    arcname = jobname + "_" + str(int(time.time()))
    dest_name = '%s.tar%s' % (arcname, dest_ext)
    dest_path = os.path.join(dest_folder, dest_name)
    if compression:
        dest_cmp =':' + compression
    else:
        dest_cmp = ''
    out = tarfile.TarFile.open(dest_path, 'w' + dest_cmp)
    out.add(folder_to_backup, arcname)
    out.close()
    content = _file_read(dest_path, 'rb')
    return dest_name, content


def _filter_files_data(files, job_info):
    """Process patch files, write the patch file to file and make the directory a gzip file.

    Args:
      files:   the patch file list
      jobinfo: current presubmit job info
    """
    if files is None:
        _error_exit("No changed files need to upload to presubmit.")
    temp_dir = tempfile.mkdtemp()
    temp_base_dir = os.path.dirname(temp_dir)
    old_current_dir = os.getcwd()
    os.chdir(temp_dir)
    _file_write("commit_msg", job_info['message'])
    user_info = '%s\n%s\n%s' % (job_info['email'], job_info['password'],
                                str(not job_info["testserver"]))
    _file_write("user_info", user_info)
    try:
        for key, values in files.items():
            os.mkdir(values[0])
            _file_write(os.path.join(values[0], "base_url"), values[1])
            _file_write(os.path.join(values[0], "revision"), values[2])
            _file_write(os.path.join(values[0], "patch"), values[3])
        return _make_tar(temp_dir, temp_base_dir, job_info['jobname'], 'gz')
    finally:
        os.chdir(old_current_dir)
        shutil.rmtree(temp_dir)
    return None, None


def _retrieve_presubmit_server_from_ci(jobname):
    """Get presubmit server from CI system by CI's python API."""
    ci_address = DEFAULT_CI_SERVER_PREFIX + jobname + DEFAULT_CI_SERVER_POSTFIX;
    try:
        response_body = urllib.urlopen(ci_address).read()
        job_info = eval(response_body)
        server_ip = job_info["builtOn"][5:]
        return server_ip + ":" + DEFAULT_PRESUBMIT_PORT;
    except Exception:
        return None


def _update_local_job_info(jobname):
    """Retrieve presubmit server from CI system by jobname through CI python api,
       and write info to ~/.last_presubmit_job_info.
    """
    if jobname is None:
        prompt = "Should specify jobname: "
        jobname = raw_input(prompt).strip()
    working_dir = os.getcwd()
    presubmit_server = _retrieve_presubmit_server_from_ci(jobname)
    if presubmit_server is None:
        _error_exit("Can't get presubmit server for job: " + jobname
                + ", please check your CI config")
    job_file_name = os.path.expanduser("~/.last_presubmit_job_info")
    if os.path.exists(job_file_name):
        os.remove(job_file_name)
    content = working_dir + "\n"
    content += jobname
    content += "\n"
    content += presubmit_server
    content += "\n"
    _file_write(job_file_name, content)
    return presubmit_server, jobname


def _retrieve_presubmit_server(jobname):
    last_job_file_name = os.path.expanduser("~/.last_presubmit_job_info")
    last_directory = ""
    last_job_name = ""
    last_server_address = ""
    if os.path.exists(last_job_file_name):
        try:
            last_job_file = open(last_job_file_name, "r")
            last_directory = last_job_file.readline().strip("\n")
            last_job_name = last_job_file.readline().strip("\n")
            last_server_address = last_job_file.readline().strip("\n")
            last_job_file.close()
        except IOError, e:
            pass
        if last_directory != find_blade_root():
            return _update_local_job_info(jobname)
        elif jobname is not None and last_job_name != jobname:
            _warning("Last save jobname is : " + last_job_name)
            prompt = "Are you sure to continue?(y/N) "
            answer = raw_input(prompt).strip()
            if answer != "y":
                _error_exit("user aborted")
            return _update_local_job_info(jobname)
        else:
            return last_server_address, last_job_name
    else:
        return _update_local_job_info(jobname)

def _encode_multi_part_form_data(fields, files):
    """Encode form fields for multipart/form-data.

    Args:
        fields: A sequence of (name, value) elements for regular form fields.
        files: A sequence of (name, filename, value) elements for data to be
            uploaded as files.

    Returns:
        (content_type, body) ready for httplib.HTTP instance.

    Source:
        http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/146306
    """
    BOUNDARY = '-M-A-G-I-C---B-O-U-N-D-A-R-Y-'
    CRLF = '\r\n'
    lines = []
    for (key, value) in fields:
        lines.append('--' + BOUNDARY)
        lines.append('Content-Disposition: form-data; name="%s"' % key)
        lines.append('')
        if isinstance(value, unicode):
            value = value.encode('utf-8')
        lines.append(value)
    for (key, filename, value) in files:
        lines.append('--' + BOUNDARY)
        lines.append('Content-Disposition: form-data; name="%s"; filename="%s"' %
                (key, filename))
        lines.append('Content-Type: %s' % _get_content_type(filename))
        lines.append('')
        if isinstance(value, unicode):
            value = value.encode('utf-8')
        lines.append(value)
        lines.append('--' + BOUNDARY + '--')
        lines.append('')
    body = CRLF.join(lines)
    content_type = 'multipart/form-data; boundary=%s' % BOUNDARY
    return content_type, body


def _main():
    if platform.python_version() < '2.6':
        _error_exit("please upgrade your python version to 2.6 or above")
    logging.basicConfig(format=("%(asctime).19s %(levelname)s %(filename)s:"
                                "%(lineno)s %(message)s "))
    check_blade_root()
    global verbosity
    cmd_options = CmdOptions()
    global options
    options = cmd_options.get_options()
    args = cmd_options.get_args()
    verbosity = options.verbose
    if verbosity >= 3:
        logging.getLogger().setLevel(logging.DEBUG)
    elif verbosity >= 2:
        logging.getLogger().setLevel(logging.INFO)

    if not options.email:
        os.environ.setdefault('USER', '')
        os.environ.setdefault('USERNAME', '')
        options.email = os.environ['USER'] or os.environ['USERNAME']
        if options.email and options.email != 'root':
            print "You do not specify an username, we set the default value as: ", options.email
        else:
            prompt = "User(outlook account): "
            options.email = raw_input(prompt).strip()
            if not options.email:
                _error_exit("A non-empty -e User is required.")

    if options.addr:
        presubmit_server = options.addr;
        jobname = options.jobname;
    else:
        presubmit_server, jobname = _retrieve_presubmit_server(options.jobname)

    if presubmit_server is None:
        _error_exit("can't retrieve presubmit server")

    if verbosity >= 1:
        print "PreSubmit server:", presubmit_server, "(change with -j/--jobname)"

    if options.issue:
        print "issue number:", options.issue
        """can't run with -i in top directory under BLADE_ROOT."""
        if os.getcwd() == find_blade_root():
            _error_exit("You can't run with -i/issue_number option in same directory with BLADE_ROOT")
    else:
        if not options.message:
            options.message = _run_editor().strip()
            if not options.message:
                _error_exit("A non-empty message is required")

    rpc_server = _get_rpc_server(presubmit_server,
                                 options.email)

    if not rpc_server.authenticated:
        rpc_server._authenticate()

    if not rpc_server.authenticated:
        _error_exit("Authenticate error, please check your username and password")


    # email, password = _get_user_credentials(options.email)
    files = {}
    _process_working_dir(files, os.getcwd(), options, args)
    job_info = {}
    job_info['email'] = options.email
    job_info['password'] = rpc_server.cookie_jar
    job_info['message'] = options.message
    job_info['jobname'] = jobname
    job_info['testserver'] = options.test_server
    gzip_file_name, gzip_data = _filter_files_data(files, job_info)
    form_fields = [("filename", gzip_file_name)]
    form_fields.append(("jobname", jobname))
    form_fields.append(("username", options.email))
    form_fields.append(("password", rpc_server.cookie_jar))
    ctype, body = _encode_multi_part_form_data(form_fields,
            [("data", gzip_file_name, gzip_data)])
    response_body = rpc_server.send("/upload", body, content_type=ctype)
    if response_body == "auth error":
        os.remove(rpc_server.cookie_file)
        _error_exit("Authenticate error, please run presubmit.py again and re-input your password.")
    if response_body != "status ok":
        _error_exit("Upload patch error, please make sure you can access the presubmit server, " + presubmit_server)
    print "Upload patch finish, please wait the presubmit build result"
    return 0


def _get_content_type(filename):
  """Helper to guess the content-type from the filename."""
  return mimetypes.guess_type(filename)[0] or 'application/octet-stream'


def main():
    error_code = 0
    try:
        error_code = _main()
    except SystemExit, e:
        error_code = e.code
    except KeyboardInterrupt:
        _error_exit("Keyboard Interrupted. ", -signal.SIGINT)
    except:
        _error_exit(traceback.format_exc())
    sys.exit(error_code)


if __name__ == "__main__":
    main()
