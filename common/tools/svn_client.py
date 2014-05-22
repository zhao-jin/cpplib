#!/usr/bin/env python
"""Copyright 2011 Tencent Inc.

Authors: Hangjun Ye <hansye@tencent.com>
         Xiaoguang Chen <sunchen@tencent.com>

A wrapper tool for svn client. It supports "client view" of a svn workspace,
which is a sub-view of a svn project and users could then fetch only code that
they need, instead of the whole svn project.

User need to write a config file named "SVN_CLIENT" under the root of svn
workspace to define which directories/externals should be fetched or excluded.

Here is an example:

# Comments start with '#' and all extra spaces/tabs are ignored.
# Define the name of svn project. It must be a svn project, instead of a
# directory.
project:
    https://tc-svn.tencent.com/search/search_search_rep/websearch_proj

# Define the views.
# Please notice the definition is order-insensitve, we always deal with include
# rules before exclude rules, regardless their order in config file.
views:
    # Fetch directory trunk/swift non-recursively. Only files under this
    # directory are fetched, other sub-dirs and their contents are skipped.
    # The trailing '/' could be omitted.
    trunk/spider/swift/

    # Fetch file trunk/spider/swift/base/BUILD.
    trunk/spider/swift/base/BUILD

    # Fetch directory trunk/falcon recursively. If it contains svn externals,
    # they are fetched recursively too.
    trunk/spider/falcon/...

    # Exclude directory trunk/falcon/tool recursively. The trailing '/' could be
    # omitted but it must be a directory, as svn does NOT support excluding a
    # file.
    -trunk/spider/falcon/tool/

    # Fetch external xfs recursively from trunk.
    trunk/:external{xfs}/...

    # Fetch external poppy non-recursively from trunk, only files under external
    # poppy are fetched.
    trunk/:external{poppy}/

    # Fetch java from external poppy recursively.
    trunk/:external{poppy}/java/...

    # Exclude directory java/lib recursively.
    -trunk/:external{poppy}/java/lib/
"""


import argparse
import hashlib
import logging
import os
import platform
import re
import shutil
import signal
import subprocess
import sys
import traceback


SVN_CLIENT_FILE = "SVN_CLIENT"
SVN_CLIENT_META_DIR = ".svn"
VIEW_RULE_REGEX = re.compile(r':(\S+)\{([^\s}]*)\}')
PEG_VERSION_REGEX = re.compile(r'([^@]+)@(\d+)')


def error_exit(print_stack):
    if not print_stack:
        sys.exit(2)
    else:
        raise RuntimeError("fatal error")

def get_cwd():
    """os.getcwd() doesn't work because it will follow symbol link.
    os.environ.get("PWD") doesn't work because it won't reflect os.chdir().
    So in practice we simply use system('pwd') to get current working directory.
    """
    return subprocess.Popen(["pwd"],
                            stdout=subprocess.PIPE,
                            shell=True).communicate()[0].strip()

def parse_command_line():
    """Command Line Options Parser"""
    parser = argparse.ArgumentParser(
        description="Manage the svn client view.")

    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Show all details.")
    parser.add_argument("--version", action="version",
                        version="%(prog)s 1.0beta")

    subparsers = parser.add_subparsers(
        title="commands",
        help="Type '%(prog)s command -h' to get more help for individual "
             "command.")
    parser_info = subparsers.add_parser(
        "info", help="Get info for client view.")
    parser_info.set_defaults(handler=process_command_info)

    parser_sync = subparsers.add_parser(
        "sync",
        help="Create/update svn client view according to SVN_CLIENT and "
             "run 'svn update' to fetch the head revision.")
    parser_sync.add_argument("-r", "--revision", type=int,
                             help="The revision number to be updated to, "
                                  "instead of the head.")
    parser_sync.set_defaults(handler=process_command_sync)

    return parser.parse_args()


# The client_root_dir is the directory which is the closest upper level
# directory of the current working directory, and containing a file
# named SVN_CLIENT.
def find_client_root_dir(working_dir):
    client_root_dir = working_dir
    if client_root_dir.endswith(os.path.sep):
        client_root_dir = client_root_dir[:-1]
    while True:
        if os.path.isfile(os.path.join(client_root_dir, SVN_CLIENT_FILE)):
            return client_root_dir
        new_root_dir = os.path.dirname(client_root_dir)
        if new_root_dir == client_root_dir:
            # Reach to the uppermost directory and don't find 'SVN_CLIENT',
            # quit now.
            logging.critical("Can't find the file 'SVN_CLIENT' in this or any "
                             "upper directory, you should create it manually "
                             "to define the client view.")
            # logging.critical doesn't terminate the process, and providing a
            # wrapper function would mess the line no where the logging call was
            # issued.
            error_exit(False)
        client_root_dir = new_root_dir

# Handlers for commands
def process_command_info(args):
    logging.critical("info command not implemented yet.")
    error_exit(False)

class SvnClientConfig:
    class SvnTreeNode:
        def __init__(self, recursive, external):
            self.leaf = False
            # If current node (directory or file) would be fetched recursively.
            self.recursive = recursive
            # If current node (directory or file) is external.
            self.external = external
            # Map of sub-node's name to its SvnTreeNode. If recursive is true,
            # this field is ignored since all sub-nodes are included.
            # If the sub-node is an external, its name possibly contains '/' as
            # it's allowed by svn, like "websearch2/proto".
            self.sub_nodes = dict()

    class PathElement:
        def __init__(self, name, external):
            self.name = name
            self.external = external
            self.sub_elements = [v for v in name.split('/') if v]

        def merge_path(self):
            return '/'.join(self.sub_elements)

    class ViewRule:
        def __init__(self, rule):
            self.exclude = False
            self.recursive = False
            self.path_elements = []
            if not self._parse_rule(rule):
                del self.path_elements[:]

        def _parse_rule(self, rule):
            if rule[0] == '-':
                self.exclude = True
                rule = rule[1:]
            if rule.endswith("/..."):
                self.recursive = True
                rule = rule[:-4]

            while rule:
                if rule[0] == ':':
                    # special directive, now support only :external{}
                    m = VIEW_RULE_REGEX.match(rule)
                    if not m: return False
                    rule = rule[len(m.group(0)):]
                    directive = m.group(1)
                    arg = m.group(2)

                    if directive == "external":
                        elem = SvnClientConfig.PathElement(arg, True)
                        if not elem.sub_elements:
                            logging.error(
                                "argument of external directive is invalid: %s",
                                arg)
                            return False
                        self.path_elements.append(elem)
                    else:
                        logging.error("unknown directive: %s", directive)
                        return False
                else:
                    pos = rule.find('/')
                    if pos >= 0:
                        elem = rule[:pos]
                        rule = rule[pos+1:]
                    else:
                        elem = rule
                        rule = ""
                    if elem:
                        self.path_elements.append(SvnClientConfig.PathElement(elem, False))

            return True

        def depth(self):
            return len(self.path_elements)

        def merge_path(self):
            return '/'.join([v.merge_path() for v in self.path_elements])


    def __init__(self, path):
        self.project = None
        self.view_root = SvnClientConfig.SvnTreeNode(False, False)
        self.checksum = None
        self.exclude_rules = []
        self._parse_config_file(path)

    def _parse_config_file(self, path):
        f = open(path, "r")
        all_rules = []  # Used for computing checksum.
        section = None
        self.line_no = 0
        for line in f:
            self.line_no += 1
            self.orig_line = line
            # Strip comments.
            pos = line.find('#')
            if pos >= 0: line = line[:pos]
            # Strip spaces/tabs.
            line = line.strip(" \t\r\n")
            # Skip empty line.
            if len(line) == 0: continue
            if line[-1] == ':':
                # Begin a section
                section = line
            else:
                # Rules of a section
                if section == "project:":
                    if self.project != None:
                        logging.critical(
                            "SVN_CLIENT:%d: duplicated project definition",
                            self.line_no)
                        error_exit(False)
                    self.project = line
                elif section == "views:":
                    all_rules.append(line)
                else:
                    logging.critical("SVN_CLIENT:%d: unknown section: %s",
                                     self.line_no, section)
                    error_exit(False)

        all_rules.sort()
        m = hashlib.md5()
        for rule in all_rules:
            self._process_view_rule(rule)
            m.update(rule)
        self.checksum = m.digest()
        self.exclude_rules.sort()

    def _process_view_rule(self, rule):
        view_rule = SvnClientConfig.ViewRule(rule)
        if not view_rule.path_elements:
            logging.critical("SVN_CLIENT:%d: invalid view definition rule: %s",
                             self.line_no, self.orig_line)
            error_exit(False)
        if view_rule.exclude:
            self.exclude_rules.append(view_rule)
        else:
            self._insert_svn_node_recursive(self.view_root, view_rule, 0)

    def _insert_svn_node_recursive(self, node, view_rule, depth):
        if node.recursive:
            # No need to add sub-node since current one has been recursive.
            return
        if depth == view_rule.depth():
            # Last node, set leaf bit and check the recursive bit
            node.leaf = True
            if view_rule.recursive:
                node.recursive = True
                node.sub_nodes.clear()
            return

        elem = view_rule.path_elements[depth]
        sub_node = node.sub_nodes.get(elem.name)
        if not sub_node:
            sub_node = SvnClientConfig.SvnTreeNode(False, elem.external)
            node.sub_nodes[elem.name] = sub_node
        self._insert_svn_node_recursive(sub_node, view_rule, depth+1)

class SvnClient:
    def __init__(self):
        # cache of the svn externals.
        self.externals = {}

    def get_info(self):
        output = subprocess.Popen(["svn", "info"],
                                  stdout=subprocess.PIPE).communicate()[0]
        info_dict = {}
        for line in output.split('\n'):
            pos = line.find(':')
            if pos < 0: continue
            info_dict[line[:pos]] = line[pos+1:].strip(" \t\r\n")
        return info_dict

    def is_svn_url(self, url):
        url_prefixes = [
            "file://",
            "http://",
            "https://",
            "svn://",
            "svn+ssh://",
            "../",
            "^/",
            "//",
            "/"
        ]
        for prefix in url_prefixes:
            if url.startswith(prefix): return True
        return False

    def get_externals(self, path):
        # There are three svn external definition styles. The first one is:
        #   third-party/sounds http://svn.example.com/repos/sounds
        #   third-party/skins -r148 http://svn.example.com/skinproj
        #   third-party/skins/toolkit -r21 http://svn.example.com/skin-maker
        #
        # The second one is:
        #   http://svn.example.com/repos/sounds third-party/sounds
        #   -r 148 http://svn.example.com/skinproj third-party/skins
        #   -r21 http://svn.example.com/skin-maker third-party/skins/toolkit
        #
        # The third one (peg version style) is:
        #   http://svn.example.com/repos/sounds third-party/sounds
        #   http://svn.example.com/skinproj@148 third-party/skins
        #   http://svn.example.com/skin-maker@21 third-party/skins/toolkit
        output = subprocess.Popen(["svn", "pget", "svn:externals", path],
                                  stdout=subprocess.PIPE).communicate()[0]
        externals_dict = {}
        for line in output.split('\n'):
            line = line.strip(" \t\r\n")
            if not line: continue
            fields = line.split()
            if len(fields) < 2 or len(fields) > 4:
                logging.warning("unknown svn external: %s", line)
                continue
            if self.is_svn_url(fields[-1]):
                # First style
                externals_dict[fields[0]] = fields[1:]
            elif self.is_svn_url(fields[0]):
                # Second or third style.
                m = PEG_VERSION_REGEX.match(fields[0])
                if m:
                    # Third style, convert to old style.
                    externals_dict[fields[-1]] = ["-r%s" % m.group(2), m.group(1)]
                else:
                    # Second style
                    externals_dict[fields[-1]] = fields[:-1]
            else:
                # Second style
                externals_dict[fields[-1]] = fields[:-1]
        return externals_dict

    def checkout(self, url, path, revision=None, depth=None):
        args = ["svn", "co", url, path]
        if revision:
            args.append("-r%d" % revision)
        if depth:
            args.append("--depth=%s" % depth)
        logging.info("checkout %s to %s, command line is %s",
                     url, path, ' '.join(args))
        subprocess.check_call(args);

    def update(self, path, revision=None, depth=None):
        args = ["svn", "up", path]
        if revision:
            args.append("-r%d" % revision)
        if depth:
            args.append("--set-depth=%s" % depth)
        logging.info("update %s, command line is %s",
                     path, ' '.join(args))
        subprocess.check_call(args)

    def checkout_external(self, base_path, external, revision=None, depth=None):
        if base_path not in self.externals:
            externals_dict = self.get_externals(base_path)
            self.externals[base_path] = externals_dict
        else:
            externals_dict = self.externals[base_path]

        args = (["svn", "co"] + externals_dict[external] +
                [base_path + '/' + external])
        if revision:
            args.append("-r%d" % revision)
        if depth:
            args.append("--depth=%s" % depth)
        logging.info("checkout external %s under %s, command line is %s",
                     external, base_path, ' '.join(args))
        subprocess.check_call(args);

    def update_external(self, base_path, external, revision=None, depth=None):
        if base_path not in self.externals:
            externals_dict = self.get_externals(base_path)
            self.externals[base_path] = externals_dict
        else:
            externals_dict = self.externals[base_path]

        args = (["svn", "up"] + externals_dict[external][:-1] +
                [base_path + '/' + external])
        if revision:
            args.append("-r%d" % revision)
        if depth:
            args.append("--set-depth=%s" % depth)
        logging.info("update external %s under %s, command line is %s",
                     external, base_path, ' '.join(args))
        subprocess.check_call(args);


class SvnClientUpdater:
    def __init__(self, previous_config, client_config, args):
        self.previous_config = previous_config
        self.client_config = client_config
        self.revision = vars(args).get("revision")
        self.svn_client = SvnClient()

    def update_svn_client(self):
        info_dict = self.svn_client.get_info()
        if not info_dict:
            # Not yet checked out, ignore the previous config and do initial
            # checkout.
            if self.previous_config:
                logging.critical(
                    "the working copy hasn't been checked out but has a "
                    "previous client config in  .svn/SVN_CLIENT, please "
                    "manually check the status.")
                error_exit(False)
            self.svn_client.checkout(self.client_config.project,
                                      ".",
                                      revision=self.revision,
                                      depth="empty")
        elif info_dict["URL"] != self.client_config.project:
            logging.critical(
                "svn project doesn't match with config: '%s' vs '%s', please "
                "manually clean up this directory or create a new client.",
                info_dict["URL"], self.client_config.project)
            error_exit(False)

        if (self.previous_config and
            self.previous_config.checksum == self.client_config.checksum):
            logging.info("no config change, just update the trunk and all "
                         "externals without setting new depth.")
            self._quick_update()
        else:
            logging.info("config changed, do the full update.")
            self._apply_include_rules()
            self._apply_exclude_rules()

    def _quick_update(self):
        # Update the root.
        self.svn_client.update(".", revision=self.revision)
        self._quick_update_externals_recursively(self.client_config.view_root, "", ".")

    def _quick_update_externals_recursively(self, node, prefix, name):
        if prefix and name:
            path = prefix + '/' + name
        else:
            path = prefix + name

        if node.external:
            self.svn_client.update_external(prefix, name)

        for sub_name, sub_node in node.sub_nodes.iteritems():
            self._quick_update_externals_recursively(
                sub_node, path, sub_name)

    def _apply_include_rules(self):
        previous_root = None
        if self.previous_config:
            previous_root = self.previous_config.view_root
        self._apply_include_rules_recursively(
            previous_root, self.client_config.view_root, "", ".")

    def _get_node_depth(self, node):
        if not node:
            return ""
        if not node.leaf:
            # internal node
            return "empty"
        if not node.recursive:
            # leaf but non-recursive
            return "files"
        # leaf and recursive
        return "infinity"

    def _apply_include_rules_recursively(self, previous_node, node, prefix, name):
        if prefix and name:
            path = prefix + '/' + name
        else:
            path = prefix + name

        if prefix:
            # Not the root, run svn update if necessary.
            # The root (i.e. the svn project) is always set depth with "empty",
            # so no need to change.
            if node.external:
                if previous_node:
                    if not previous_node.external:
                        logging.critical(
                            "The external is NOT an external in previous "
                            "client config, please manually check it: %s, %s.",
                            prefix, name)
                else:
                    # external and we didn't check out it previously, do check
                    # out firstly
                    self.svn_client.checkout_external(
                        prefix, name, depth="empty")

            depth = self._get_node_depth(node)
            previous_depth = self._get_node_depth(previous_node)
            if depth == previous_depth: depth = None
            if node.external:
                self.svn_client.update_external(prefix, name, depth=depth)
            else:
                self.svn_client.update(path, revision=self.revision, depth=depth)

        for sub_name, sub_node in node.sub_nodes.iteritems():
            # Find the corresponding node in previous client config firstly.
            previous_sub_node = None
            if previous_node:
                previous_sub_node = previous_node.sub_nodes.get(sub_name)
            self._apply_include_rules_recursively(
                previous_sub_node, sub_node, path, sub_name)

    def _apply_exclude_rules(self):
        # TODO: if user remove an exclude rule, it doesn't work now, fix it.
        previous_exclude_rules_set = set()
        if self.previous_config:
            for rule in self.previous_config.exclude_rules:
                previous_exclude_rules_set.add(rule.merge_path())

        for rule in self.client_config.exclude_rules:
            path = rule.merge_path()
            if path not in previous_exclude_rules_set:
                self.svn_client.update(path, depth="exclude")


def process_command_sync(args):
    # Load previous client config.
    try:
        previous_config = SvnClientConfig(
            SVN_CLIENT_META_DIR + "/" + SVN_CLIENT_FILE)
    except IOError, e:
        previous_config = None

    # Load current client config.
    client_config = SvnClientConfig(SVN_CLIENT_FILE)

    # Update svn client view.
    client_updater = SvnClientUpdater(previous_config, client_config, args)
    client_updater.update_svn_client()

    # Copy current config to ".svn/.svn_config" as a cache.
    shutil.copyfile(SVN_CLIENT_FILE,
                    SVN_CLIENT_META_DIR + "/" + SVN_CLIENT_FILE)
    return 0


def main_entry():
    """The main entry"""
    # TODO: pass log level from command line option.
    logging.basicConfig(
        level=logging.INFO,
        format="[%(levelname)s] %(asctime)-15s %(filename)s:%(lineno)d %(message)s")

    # Check the python version
    if platform.python_version() < "2.7":
        logging.critical("please update your python version to 2.7 or above")
        error_exit(False)

    args = parse_command_line()

    # Find the client root dir and change current dir to there.
    working_dir = get_cwd()
    client_root_dir = find_client_root_dir(working_dir)
    os.chdir(client_root_dir)
    if client_root_dir != working_dir:
        logging.info("svn_client: Entering directory '%s'", client_root_dir)

    return args.handler(args)


def main():
    try:
        main_entry()
    except:
    # TODO: catch some execptions here to make the error message more readable.
        raise


if __name__ == "__main__":
    main()
