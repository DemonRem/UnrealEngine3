import sys
import os
import os.path
import fnmatch

project_extension = "vcproj"

args = sys.argv[1:]
if not args:
    print "Usage: %s [pattern] ..." % sys.argv[0]
    print "Example: %s *.uc" % sys.argv[0]
    sys.exit()

def find_files_matching_patterns(patterns, dir=sys.argv[2]):
    matching_files = []
    for root, dirs, files in os.walk(dir):
        for file in files:
            for pattern in patterns:
                if fnmatch.fnmatch(file, pattern):
                    matching_files.append((file, os.path.join(root, file)))
                    break
    return matching_files


# here we set a flag and then look for missing files
# if we find any missing files we set increment the flag and then will return that value
foundMissingFile = 0

combined_project_str = "".join(open(path).read() for file, path in find_files_matching_patterns(["*." + project_extension])).lower()
for file, path in find_files_matching_patterns(args):
    if file.lower() not in combined_project_str:
        print "MissingFile: %s" % path
        foundMissingFile += 1

if foundMissingFile > 0:
    print "Found %s missing files" % foundMissingFile
    sys.exit(foundMissingFile)
