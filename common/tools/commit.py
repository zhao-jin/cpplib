#!/usr/bin/env python

"""
Auto commit script.
Authors: Huan Yu <huanyu@tencent.com>
         Chen Feng <phongchen@tencent.com>
"""

from optparse import OptionParser
import hashlib
import json
import optparse
import os
import subprocess
import sys
import tempfile
import urllib
import urllib2

sys.path.append(os.path.dirname(__file__))
import upload

_EXT_IGNORES = frozenset([
    '.bak',
    '.diff',
    '.log',
    '.new',
    '.old',
    '.orig',
    '.tmp',
])


def _error_exit(msg, code=1):
    msg = '\033[1;31m' + msg + '\033[0m'
    print >>sys.stderr, msg
    sys.exit(code)


def _warning(msg):
    msg = '\033[1;33m' + msg + '\033[0m'
    print >>sys.stderr, msg


def _run_command_in_english(cmdlist):
    env = os.environ
    env["LC_ALL"] = "en_US.UTF-8"
    return subprocess.Popen(cmdlist,
                            stdout=subprocess.PIPE,
                            env=env).communicate()[0]


def find_file_bottom_up(filename, from_dir='.'):
    """find_blade_root_dir to find the dir holds the BLADE_ROOT file.

    The finding_dir is the directory which is the closest upper level
    directory of the current working directory, and containing a file
    named BLADE_ROOT.

    """
    finding_dir = os.path.abspath(from_dir)
    while True:
        path = os.path.join(finding_dir, filename)
        if os.path.isfile(path):
            return path
        if finding_dir == '/':
            break
        finding_dir = os.path.dirname(finding_dir)
    return ''


class SvnInfo(object):
    def __init__(self, path):
        info = _run_command_in_english(["svn", "info", path])
        self.svn_info = "\n".join(info.split("\n")[1:])

    def get_remote_path(self):
        for i in self.svn_info.split("\n"):
            if i.startswith("URL: "):
                return i[5:].strip()
        return ""

    def get_revision(self):
        for i in self.svn_info.split("\n"):
            if i.startswith("Revision: "):
                return i.split(":")[1].strip()
        return ""

    def get_last_changed_revision(self):
        for i in self.svn_info.split("\n"):
            if i.startswith("Last Changed Rev: "):
                return i.split(":")[1].strip()
        return ""


def svn_status(path):
    status = _run_command_in_english(["svn", "status", path])
    return [tuple(line.split()) for line in status.splitlines()]


class CodeReviewServer(object):
    def __init__(self, site, username):
        upload.options = optparse.Values()
        upload.options.email = username
        self.__rpc_server = upload.GetRpcServer(site, username, save_cookies=True)
        self.__token = self.__get_xsrf_token()

    def __get_xsrf_token(self):
        r = self.__rpc_server.Send('/')
        for line in r.splitlines():
            if 'var xsrfToken = ' in line:
                return line.split("'")[1]
        return ''

    def __post_issue_call(self, issue, function, form_fields, files=None):
        if files is None:
            files = []
        ctype, body = upload.EncodeMultipartFormData(form_fields, files)
        try:
            self.__rpc_server.Send(
                    '/%s/%s' % (issue, function),
                    body,
                    content_type=ctype,
                    ignore_302=True)
        except urllib2.HTTPError, e:
            _warning('%s issue %s failed: %s, %s' % (function, issue, e, e.read()))
            return False
        return True

    def add_comment(self, issue, subject, message, reviewers, cc):
        form_fields = [
            ('xsrf_token', self.__token),
            ('subject', subject),
            ("reviewers", ','.join(reviewers)),
            ("cc", ','.join(cc)),
            ("send_mail", 'on'),
            ("message", message),
            ("message_only", '1'),
            ('no_redirect', '1')
        ]
        return self.__post_issue_call(issue, 'publish', form_fields)

    def close_issue(self, issue):
        form_fields = [
                ('xsrf_token', self.__token),
                ('no_redirect', '1')]
        return self.__post_issue_call(issue, 'close', form_fields)


class IssueCommitter(object):
    def __init__(self, options):
        self.options = options
        self.issue = options.issue
        self.issue_info_url = "%s/%s/info" % (upload.DEFAULT_REVIEW_SERVER, options.issue)
        self.issue_url = "%s/%s" % (upload.DEFAULT_REVIEW_SERVER, options.issue)
        self.user = os.environ.get('USER') or os.environ.get('USERNAME')
        self.issue_info = json.load(urllib.urlopen(self.issue_info_url))
        try:
            self.issue_info = json.load(urllib.urlopen(self.issue_info_url))
        except:
            _error_exit("Invalid issue or server down")
        self.local_svn_info = SvnInfo('.')
        self.remote_svn_info = SvnInfo(self.local_svn_info.get_remote_path())
        self.svn_status = svn_status('.')
        self.codereview_server = CodeReviewServer(upload.DEFAULT_REVIEW_SERVER,
                self.user)

    def check_svn_up_to_date(self):
        local_revision = self.local_svn_info.get_last_changed_revision()
        remote_revision = self.remote_svn_info.get_last_changed_revision()
        if local_revision != remote_revision:
            _error_exit('Your codebase is not up-to-date, '
                        'please svn update to head.')

    def check_svn_missing(self):
        def maybe_missing(status):
            if not '?' in status:
                return False
            filename = status[1]
            ext = os.path.splitext(filename)[1]
            return ext not in _EXT_IGNORES

        missings = [st[1] for st in filter(maybe_missing, self.svn_status)]
        if missings:
            prompt = ('The following files has not been added to svn:\n'
                      '%s\n'
                      'Continue?(y/N) ' %
                      '\n'.join(missings))
            answer = raw_input(prompt).strip()
            if answer != 'y':
                _error_exit('Exit')

    def check_has_unreviewed_changes(self):
        svn_status = _run_command_in_english(["svn", "status"]).split("\n")
        local_changes = []
        for line in svn_status:
            if line.startswith('M'):
                filename = line.split()[1]
                local_changes.append(filename)
        reviewed_changes = self.issue_info["filenames"]
        unreviewed_changes = set(local_changes) - set(reviewed_changes)
        if len(unreviewed_changes) != 0:
            prompt = ('The following files has not been reviewed :\n'
                      '%s\n'
                      'Continue?(y/N) ' %
                      '\n'.join(unreviewed_changes))
            answer = raw_input(prompt).strip()
            if answer != 'y':
                _error_exit('Exit')

    def _is_same_url(self, url1, url2):
        if url1 == url2:
            return True
        if len(url1) == len(url2) + 1:
            return url1[0:-1] == url2 and url1[-1:] == '/'
        if len(url1) + 1 == len(url2):
            return url1 == url2[0:-1] and url2[-1:] == '/'
        return False

    def check_approve(self):
        if (self.user != self.issue_info["owner"] and
            self.user not in self.issue_info["approvers"]):
            _error_exit("You are not the owner or approver of this issue")

        if not self.issue_info['approvers']:
            _error_exit("Not approved.")

    def check_path(self):
        work_url = self.local_svn_info.get_remote_path()
        issue_base = self.issue_info["base_url"]
        if not self._is_same_url(work_url, issue_base):
            _error_exit("Different path:\nlocal is %s\nissue is %s" %
                    (work_url, issue_base))

    def build_and_runtests(self):
        if self.options.no_test:
            return
        p = subprocess.Popen("blade test --generate-dynamic -p%s -t %s ..." % (
                             self.options.profile, self.options.test_jobs),
                             shell=True)
        status = os.waitpid(p.pid, 0)[1]
        if status:
            sys.exit(1)

    def run_presubmit(self):
        presubmit = find_file_bottom_up('presubmit')
        if presubmit and os.access(presubmit, os.X_OK):
            p = subprocess.Popen(presubmit, shell=True)
            if p.wait() != 0:
                _error_exit('presubmit error')
        else:
            self.build_and_runtests()

    def add_missing_dir(self, files):
        """
        If a new dir added to svn, uploay.py can only upload files but not dir.
        Add dir if not in files"""
        dirs = set()
        for f in files:
            for st in self.svn_status:
                if (len(st) < 2):
                    continue
                sf = st[1]
                if st[0] == 'A' and f.startswith(sf) and os.path.isdir(sf):
                    dirs.add(sf)
        # sort enforce parent dir before child dir if multiple level dir added,
        # svn commit require this.
        dirs = list(dirs)
        dirs.sort()
        return dirs + files

    def commit_to_svn(self):
        title = self.issue_info["subject"]
        (fd, path) = tempfile.mkstemp("svn_commit")
        f = open(path, "w")
        print >>f, title.encode('utf-8')
        print >>f, "Issue: %s" % self.issue_url
        m = hashlib.md5()
        m.update(title.encode('UTF-8'))
        m.update(self.issue_url)
        print >>f, "Digest: %s" % m.hexdigest()
        f.close()
        os.close(fd)
        filenames = '.'
        if not self.options.whole_dir:
            files = self.add_missing_dir(self.issue_info["filenames"])
            filenames = " ".join(files)

        cmd = "svn commit -F %s %s" % (path, filenames)
        p = subprocess.Popen(cmd, shell=True)
        p.wait()
        os.remove(path)
        if p.returncode != 0:
            _error_exit("Not committed")

    def svn_update(self):
        """ Update again after commit to sync with version library"""
        stdout = _run_command_in_english(['svn', 'update'])
        return stdout.splitlines()[-1].strip().replace('At revision ', '').replace('.', '')

    def add_comment_to_board(self, revision):
        if self.codereview_server.add_comment(self.issue,
                self.issue_info['subject'], 'Committed revision %s.' % revision,
                self.issue_info['reviewers'], self.issue_info['cc']):
            print 'Add committed message to issue %s.' % self.issue
        else:
            _warning('Add commit comment failed for issue %s, you should add '
                     'it manualy, please tell phongchen this bad message.' %
                     self.issue)

    def close_issue(self):
        if self.codereview_server.close_issue(self.issue):
            print 'Issue %s closed.' % self.issue
        else:
            _warning('Issue %s close failed, you must close it manual;y, '
                     'please tell phongchen this sad message.' % self.issue)


def main():
    parser = OptionParser()
    parser.add_option("-i", "--issue", dest="issue",
                      help="Codereview issue number.")
    parser.add_option("-n", "--no-test", dest="no_test", action="store_true",
                      help="Don't build and test before commit.")
    parser.add_option("-p", "--profile", dest="profile", default="release",
                      help="Debug or release mode for build test")
    parser.add_option("-t", "--test-jobs", dest="test_jobs", type=int,
                      default=1, help="Number of concurrent test jobs")
    parser.add_option("-w", "--whole-dir", dest="whole_dir", action="store_true",
                      help="Submit the whole dir instead changed files only")

    (options, args) = parser.parse_args()
    if not options.issue:
        parser.print_help()
        _error_exit("")

    committer = IssueCommitter(options)

    committer.check_path()
    committer.check_svn_missing()
    committer.check_has_unreviewed_changes()
    committer.check_svn_up_to_date()
    committer.check_approve()
    committer.run_presubmit()

    committer.commit_to_svn()

    revision = committer.svn_update()
    committer.add_comment_to_board(revision)
    committer.close_issue()


if __name__ == "__main__":
    main()
