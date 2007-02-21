#!/usr/bin/python
"""Maintains the Check for Updates web pages.

Automatically generates all updates web pages (one per released version) based
on the first version in the list.

A version web page contains release notes for all intermediate versions, so that
a user can easily see *everything* that's changed since.
"""
__author__ = "Kevin Grant <kevin@ieee.org>"
__date__ = "28 September 2006"

import sys, os
import os.path as path
from Resources.English.release_notes import version_lineage, daily_build_lineage, notes_by_version

def subst_line(line, vars):
    """subst_line(line, vars) -> string
    
    Return a string with variable substitutions applied.
    
    If the specified line contains '(' and ')', Python's string
    substitutions are applied to it using the given variable map
    (e.g. %(foobar)s becomes whatever value 'foobar' has).  Passing
    the locals() map is recommended.
    """
    # requiring parentheses helps avoid confusing Python
    # for lines that contain only percents (like URLs)
    if '(' in line and ')' in line:
        result = str(line) % vars
    else:
        result = line
    return result

def write_subst(ofh, line, vars):
    """write_subst(ofh, line, vars)
    
    Like subst_line(), but prints the substituted value to the
    specified output file handle "ofh".  No newline is added, it is
    assumed that the given line already has one.
    """
    printed_line = subst_line(line, vars)
    print >>ofh, printed_line, # <- trailing ',' quells extra newline

def generate_version_history_file(version, ofh, template_lines, recent_versions):
    """generate_version_history_file(version, ofh, template_lines, recent_versions)
    
    
    """
    main_lines = []
    update_loop = False
    version_loop = False
    release_notes_loop = False
    no_updates_loop = False
    # read once to find all template loop fragments
    for line in template_lines:
        if '<!-- BEGIN NEW UPDATE -->' in line:
            main_lines.append(line) # used as insertion point later
            update_loop = True
            update_loop_lines = []
        elif '<!-- BEGIN VERSION LOOP -->' in line:
            update_loop_lines.append(line) # used as insertion point later
            version_loop = True
            version_loop_lines = []
        elif '<!-- BEGIN RELEASE NOTES LOOP -->' in line:
            version_loop_lines.append(line) # used as insertion point later
            release_notes_loop = True
            release_notes_loop_lines = []
        elif '<!-- BEGIN NO UPDATES -->' in line:
            main_lines.append(line) # used as insertion point later
            no_updates_loop = True
            no_updates_loop_lines = []
        # IMPORTANT: search for end markers before processing loops!
        elif '<!-- END NO UPDATES -->' in line:
            no_updates_loop = False
        elif '<!-- END RELEASE NOTES LOOP -->' in line:
            release_notes_loop = False
        elif '<!-- END VERSION LOOP -->' in line:
            version_loop = False
        elif '<!-- END NEW UPDATE -->' in line:
            update_loop = False
        elif release_notes_loop is True:
            release_notes_loop_lines.append(line)
        elif version_loop is True:
            version_loop_lines.append(line)
        elif update_loop is True:
            update_loop_lines.append(line)
        elif no_updates_loop is True:
            no_updates_loop_lines.append(line)
        else:
            main_lines.append(line)
    # now use the fragments to generate a specific-version file
    for line in main_lines:
        if '<!-- BEGIN NEW UPDATE -->' in line and len(recent_versions) > 0:
            for update_line in update_loop_lines:
                if '<!-- BEGIN VERSION LOOP -->' in update_line:
                    for version in recent_versions:
                        version_notes = notes_by_version[version]
                        for version_line in version_loop_lines:
                            if '<!-- BEGIN RELEASE NOTES LOOP -->' in version_line:
                                for release_note in version_notes:
                                    for note_line in release_notes_loop_lines:
                                        write_subst(ofh, note_line, locals())
                            else:
                                write_subst(ofh, version_line, locals())
                else:
                    write_subst(ofh, update_line, locals())
        elif '<!-- BEGIN NO UPDATES -->' in line and len(recent_versions) == 0:
            for no_updates_line in no_updates_loop_lines:
                write_subst(ofh, no_updates_line, locals())
        elif '<!-- END' in line:
            pass
        else:
            write_subst(ofh, line, locals())

src_dir = os.environ['RELEASE_NOTES_SRC']
dest_dir = os.environ['RELEASE_NOTES_DEST']
template_index = 'template-index.html'
template_index_path = path.join(src_dir, template_index)
template_version = 'template-version.html'
template_version_path = path.join(src_dir, template_version)
template_daily = 'template-daily.html'
template_daily_path = path.join(src_dir, template_daily)

if __name__ == '__main__':
    # by default, all versions are returned; or, specify which ones
    if len(sys.argv) > 1: lineage = sys.argv[1:]
    else: lineage = version_lineage
    
    # read index page's template lines
    index_in = file(template_index_path)
    index_lines = index_in.readlines()
    index_in.close()
    
    # write an index file based on actual versions
    index_out = file(path.join(dest_dir, "index.html"), 'w')
    version_loop = False
    for line in index_lines:
        if '<!-- BEGIN VERSION LOOP -->' in line:
            version_loop = True
        elif version_loop:
            for version in lineage:
                write_subst(index_out, line, locals())
        elif '<!-- END VERSION LOOP -->' in line:
            version_loop = False
        else:
            write_subst(index_out, line, locals())
    index_out.close()
    
    # read version page's template lines
    version_in = file(template_version_path)
    version_lines = version_in.readlines()
    version_in.close()
    
    # write one file per release
    for i, version in enumerate(lineage):
        version_out = file(path.join(dest_dir, "%s.html" % version), 'w')
        generate_version_history_file(version, version_out, version_lines, lineage[:i])
        version_out.close()
    
    # read daily build page's template lines
    daily_in = file(template_daily_path)
    daily_lines = daily_in.readlines()
    daily_in.close()
    
    # write a file for the daily build
    full_lineage = daily_build_lineage[:]
    full_lineage.extend(lineage)
    daily_out = file(path.join(dest_dir, "daily.html"), 'w')
    generate_version_history_file(daily_build_lineage[0], daily_out, daily_lines, full_lineage)
    daily_out.close()

