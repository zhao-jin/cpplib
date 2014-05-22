// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>
//
// Description: This is a small tool to generate resource file definations for vsprojects.

#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

bool ReadFileList(const std::string& filename, std::vector<std::string>* vec)
{
    std::ifstream instream;
    instream.open(filename.c_str());
    if (!instream) {
        return false;
    }
    vec->clear();
    std::string line;
    while (!instream.eof()) {
        std::getline(instream, line);
        if (line == "") continue;
        vec->push_back(line);
    }
    instream.close();
    return true;
}

int GetFileSize(const char* filename)
{
    std::ifstream in(filename);
    if (!in) {
        return -1;
    }
    in.seekg(0, std::ios::end);
    std::streampos ps = in.tellg();
    in.close();
    return ps;
}

bool GetResourceName(const std::string& fullname, std::string* name)
{
    std::string::size_type pos = fullname.find_last_of('\\');
    if (pos == std::string::npos) {
        return false;
    }
    *name = fullname.substr(pos + 1);
    for (size_t i = 0; i < name->size(); ++i) {
        if (!isalpha(name->at(i)) && !isdigit(name->at(i))) {
            name->at(i) = '_';
        }
    }
    return true;
}

bool WriteHeader(const std::string& path, const std::vector<std::string>& files)
{
    std::ofstream header;
    std::string output_file = path + "\\static_resource.h";
    header.open(output_file.c_str());
    if (!header) {
        return false;
    }

    header << "#ifndef POPPY_STATIC_RESOURCE_H_" << std::endl;
    header << "#define POPPY_STATIC_RESOURCE_H_" << std::endl;
    for (size_t i = 0; i < files.size(); ++i) {
        int len = GetFileSize(files[i].c_str());
        std::string name;
        if (len < 0 || !GetResourceName(files[i], &name)) {
            header.close();
            return false;
        }
        header << "extern const unsigned char RESOURCE_poppy_static_"
               << name << "[" << len << "];" << std::endl;
    }
    header << "#endif" << std::endl;

    header.close();
    return true;
}

std::string ToHexString(unsigned char c)
{
    static const char hexdigits[] = "0123456789abcdef";
    std::string result = "0x";
    result += hexdigits[c/16];
    result += hexdigits[c%16];
    return result;
}

bool WriteFile(const std::string& filename, std::ofstream& ostream)
{
    std::string name;
    if (!GetResourceName(filename, &name)) {
        return false;
    }
    std::ifstream in(filename.c_str());
    if (!in) {
        return false;
    }
    ostream << "extern const unsigned char RESOURCE_poppy_static_" << name << "[] = {\n";
    int count = 0;
    char c;
    while (in.get(c)) {
        if (count != 0 && count % 12 == 0) {
            ostream << "\n";
        }
        ostream << " " << ToHexString(c) << ",";
        ++count;
    }
    in.close();
    ostream << "\n};\n\n";
    return true;
}

bool WriteSource(const std::string& path, const std::vector<std::string>& files)
{
    std::ofstream source;
    std::string output_file = path + "\\static_resource.cc";
    source.open(output_file.c_str());
    if (!source) {
        return false;
    }
    for (size_t i = 0; i < files.size(); ++i) {
        if (!WriteFile(files[i], source)) {
            source.close();
            return false;
        }
    }
    source.close();
    return true;
}

bool NeedUpdate(const std::vector<std::string>& source_files, const std::string& root_path) {
    time_t last_resource_mt = 0;
    for (size_t i = 0; i < source_files.size(); ++i) {
        struct stat buf = {0};
        stat(source_files[0].c_str(), &buf);
        if (buf.st_mtime > last_resource_mt)
            last_resource_mt = buf.st_mtime;
    }

    time_t last_resource_cpp_mt = 0;
    struct stat buf = {0};
    std::string rs_h = root_path + "\\static_resource.h";
    stat(rs_h.c_str(), &buf);
    if (buf.st_mtime > last_resource_cpp_mt)
        last_resource_cpp_mt = buf.st_mtime;

    memset(&buf, 0, sizeof(buf));
    std::string rs_cc = root_path + "\\static_resource.cc";
    stat(rs_cc.c_str(), &buf);
    if (buf.st_mtime > last_resource_cpp_mt)
        last_resource_cpp_mt = buf.st_mtime;

    return last_resource_mt >= last_resource_cpp_mt;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "build_resource.exe <root_path> <source_file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string root_path = argv[1];
    std::string source_file = argv[2];

    std::vector<std::string> files;
    if (!ReadFileList(source_file, &files)) {
        std::cerr << "Failed to read resource file: " << source_file << std::endl;
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < files.size(); ++i) {
        files[i] = "\\" + files[i];
        files[i] = root_path + files[i];
    }

    if (!NeedUpdate(files, root_path)) {
        std::cerr << "don't need update resource." << std::endl;
        return EXIT_SUCCESS;
    }

    if (!WriteHeader(root_path, files)) {
        std::cerr << "Failed to generate static_resource.h" << std::endl;
        return EXIT_FAILURE;
    }
    if (!WriteSource(root_path, files)) {
        std::cerr << "Failed to generate static_resource.cc" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
