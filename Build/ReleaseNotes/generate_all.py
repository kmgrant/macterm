#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Maintains the Check for Updates web pages.

Automatically generates all updates web pages (one per released version) based
on the first version in the list.

A version web page contains release notes for all intermediate versions, so that
a user can easily see *everything* that's changed since.

"""
__author__ = "Kevin Grant <kmg@mac.com>"
__date__ = "28 September 2006"

import sys
import os
import os.path as path
import re
from Resources.English.release_notes import \
     (version_lineage,
      daily_build_lineage,
      notes_by_version)

def subst_line(line, variables):
    """subst_line(line, variables) -> string
    
    Return a string with variable substitutions applied.
    
    If the specified line contains '(' and ')', Python's string
    substitutions are applied to it using the given variable map
    (e.g. %(foobar)s becomes whatever value 'foobar' has).  Passing
    the locals() map is recommended.
    
    """
    # requiring parentheses helps avoid confusing Python
    # for lines that contain only percents (like URLs)
    if '(' in line and ')' in line:
        result = str(line) % variables
    else:
        result = line
    return result

def write_subst(ofh, line, variables):
    """write_subst(ofh, line, variables)
    
    Like subst_line(), but prints the substituted value to the
    specified output file handle "ofh".  No newline is added, it is
    assumed that the given line already has one.
    
    """
    printed_line = subst_line(line, variables)
    print >>ofh, printed_line, # <- trailing ',' quells extra newline

def write_one_section_line(ofh, line, section, sub_vars, recent_versions):
    """write_one_section_line(ofh, line, section, sub_vars, recent_versions)

    Conditionally generates up to one line of output belonging to
    the specified section using the given dictionary as a guide.  

    The line may represent one iteration of a loop section or it may
    not be written at all (e.g. because the conditions for writing
    lines from its section were not met).
    
    """
    if (section == 'YEAR'):
        write_subst(ofh, line, sub_vars)
    elif (section == 'YEAR-LINK'):
        write_subst(ofh, line, sub_vars)
    elif (section == 'VERSION'):
        write_subst(ofh, line, sub_vars)
    elif (section == 'RELEASE-NOTES'):
        if len(recent_versions) > 0:
            write_subst(ofh, line, sub_vars)
    elif (section == 'NO-UPDATES'):
        if len(recent_versions) == 0:
            write_subst(ofh, line, sub_vars)
    elif (section == 'NEW-UPDATE'):
        if len(recent_versions) > 0:
            write_subst(ofh, line, sub_vars)
    else:
        write_subst(ofh, line, sub_vars)

def generate_html(ofh, template_lines, subst_kv, **kwargs):
    """generate_html(ofh, template_lines, config, **kwargs)
    
    Writes a version of the template lines to the given file.

    The 'kwargs' dictionary may provide hints as to the type of HTML
    document being produced:

    - 'recent_versions'
      An array of versions that should be included in the document.
      For a "daily build" log this will basically be *every* dated
      version; for any other version-specific document this will be
      a list of all versions between the user's current version and
      the latest available version.  N/A for other document types.

    Template lines may contain several special markers that affect
    the behavior of the generator:

    - Python-style %-references that refer to settings in 'subst_kv'.
      You may populate this dictionary in advance but other values
      may be added (see below) based on the section(s) and/or loop(s)
      that directly or indirectly contain a template line.

    - '<!-- BEGIN:MY-SECTION -->' and '<!-- END:MY-SECTION -->'
      bracket lines in the section 'MY-SECTION'.  A set of variables
      is defined according to the section name (in this case,
      'MY-SECTION').  In addition, NESTED sections make available ALL
      variables from parent contexts.

    - '<!-- BEGIN-LOOP:MY-LOOP -->' and '<!-- END-LOOP:MY-LOOP-->'
      bracket lines in the looping section 'MY-LOOP'.  A set of
      variables is defined PER ITERATION according to the loop name
      (in this case, 'MY-LOOP').  In addition, NESTED sections make
      available ALL variables from parent contexts; if a parent is a
      loop then implicitly the inherited value is the current
      iteration of the loop.

    - The pattern '<http://...>' is replaced by HTML that links to
      the specified URL.

    - The pattern '(Trac #123)' is replaced by HTML that links to
      the web page for the given Trac ticket number.

    The section and/or loop hierarchy is as follows (lower levels
    implicitly have access to variables defined by parents):

      NO-UPDATES
       '--- (none)
      NEW-UPDATE
       |--- VERSION [LOOP]
       |     '--- RELEASE-NOTES [LOOP]
       |--- YEAR-LINK [LOOP]
       '--- YEAR [LOOP]
             '--- VERSION [LOOP]
                   '--- RELEASE-NOTES [LOOP]

      A template file may define any sections and loops it wants but
      only the patterns above guarantee that you will be able to
      inherit the corresponding parents' variables.

      A NEW-UPDATE section contains a description of a single latest
      version of MacTerm.  It can be (but is not required to be)
      coupled with a parallel NO-UPDATES section that the user would
      see if their current version was already up-to-date.

      A VERSION loop contains lines that are repeated for all the
      versions that apply to the current document.  It is very common
      for a sub-loop RELEASE-NOTES to be defined that contains all
      the release notes applicable to a version.

      Sometimes a VERSION loop has a YEAR loop as a parent (this is
      for daily builds, which follow a date pattern).  This is useful
      for organizing versions according to their year of release.

    The following special keys take precedence over 'subst_kv' and
    they are only guaranteed to be defined in the given [SECTION]
    (some may also appear outside any section):

    - 'current_version' [global]
      If the template's purpose is to represent a single version of
      MacTerm then this is that version.

    - 'release_note' [RELEASE-NOTES]
      The text for a release note in an iteration.

    - 'version' [global, VERSION]
      A version in a VERSION loop or a version for a whole file.

    - 'year' [YEAR, YEAR-LINK]
      A 4-digit year in a YEAR loop.

    - 'year_heading_id' [YEAR-LINK, YEAR]
      An HTML element name that will uniquely identify a heading
      element belonging to the year of an iteration.  This is useful
      for linking and styling.

    - 'year_body_id' [YEAR]
      An HTML element name that will uniquely identify a section-body
      element belonging to the year of an iteration.  This is useful
      for scripting and styling.

    - 'year_element_display' [YEAR]
      A CSS display type such as 'block'.  This should be used as
      the display style of a section-body element belonging to the
      year of an iteration.  Scripts can change the display value
      later in response to user actions.

    - 'year_element_displayer_remover' [YEAR]
      A CSS style.  This should be used as the display style of a
      heading element belonging to the year of an iteration.
    
    """
    def build_section(section, variables):
        def write_section_lines():
            for line in lines_by_section[section]:
                # a list type is just a trick for embedding section references;
                # the list should always contain one element, a section name
                if isinstance(line, list):
                    build_section(line[0], sub_vars)
                else:
                    rv = []
                    if 'recent_versions' in kwargs: rv = kwargs['recent_versions']
                    write_one_section_line(ofh, line, section, sub_vars, rv)
        def start_section():
            # give sections a chance to set up; for loops, this is
            # where the actual looping occurs
            if (section == 'NEW-UPDATE'):
                # not a loop
                for version in kwargs['recent_versions']:
                    try:
                        # this applies to daily build versions only, e.g. '20120101'
                        year = str(int(version[0:4]))
                        if 'versions_by_year' not in sub_vars:
                            sub_vars['versions_by_year'] = dict()
                        if year not in sub_vars['versions_by_year']:
                            sub_vars['versions_by_year'][year] = []
                            if 'year_order' not in sub_vars:
                                sub_vars['year_order'] = []
                            sub_vars['year_order'].append(year)
                        sub_vars['versions_by_year'][year].append(version)
                    except ValueError, e:
                        # this is raised for regular versions, e.g. '4.0.0' (ignore)
                        pass
                write_section_lines()
            elif (section == 'YEAR-LINK'):
                # loop; must be within the 'NEW-UPDATE' section
                if ('year_order' in sub_vars) and (len(sub_vars['year_order']) > 0):
                    # daily builds
                    for year in sub_vars['year_order']:
                        sub_vars['year'] = year
                        sub_vars['year_heading_id'] = "year%s_top" % year
                        write_section_lines()
            elif (section == 'YEAR'):
                # loop; must be within the 'NEW-UPDATE' section
                if ('year_order' in sub_vars) and (len(sub_vars['year_order']) > 0):
                    # daily builds
                    year_index = 0
                    for year in sub_vars['year_order']:
                        sub_vars['year_element_displayer_remover'] = "element-remover"
                        sub_vars['year_element_display'] = "block"
                        sub_vars['year'] = year
                        if year_index > 0:
                            # show other years by default (could also hide them
                            # by using "none" and "element-displayer" for these)
                            sub_vars['year_element_display'] = "block"
                            sub_vars['year_element_displayer_remover'] = "element-remover"
                        sub_vars['year_heading_id'] = "year%s_top" % year
                        sub_vars['year_body_id'] = "year%s_body" % year
                        write_section_lines()
                        year_index = year_index + 1
            elif (section == 'VERSION'):
                if 'versions_by_year' in sub_vars:
                    # loop; must be within a 'YEAR' loop
                    for version in sub_vars['versions_by_year'][sub_vars['year']]:
                        sub_vars['version'] = version
                        sub_vars['version_notes'] = notes_by_version[version]
                        write_section_lines()
                else:
                    # loop; must be within the 'NEW-UPDATE' section
                    for version in kwargs['recent_versions']:
                        sub_vars['version'] = version
                        sub_vars['version_notes'] = notes_by_version[version]
                        write_section_lines()
            elif (section == 'RELEASE-NOTES'):
                # loop; must be within a 'VERSION' loop
                if 'version_notes' in sub_vars:
                    for release_note in sub_vars['version_notes']:
                        sub_vars['release_note'] = http_link_regex.sub(r'<a href="\1">\1</a>', release_note)
                        sub_vars['release_note'] = trac_link_regex.sub(r'(<a href="http://sourceforge.net/apps/trac/mactelnet/ticket/\1">Trac #\1</a>)', release_note)
                        write_section_lines()
            else:
                write_section_lines()
        # create a copy of the dictionary so that mappings can be added
        # to the context of recursive calls without affecting parents;
        # then set up the specified section and write its lines (this
        # triggers recursive calls for any embedded section references)
        sub_vars = {v: variables[v] for v in variables}
        start_section()
    # parse the template
    global current_line
    lines_by_section = dict()
    # read once to find all template loop fragments
    section_stack = []
    default_section = '__root__';
    section_stack.append(default_section)
    lines_by_section[section_stack[-1]] = []
    current_line = 0
    for line in template_lines:
        current_line = current_line + 1
        is_begin = begin_section_regex.findall(line)
        is_loop = begin_loop_regex.findall(line)
        is_end = end_section_regex.findall(line)
        is_loop_end = end_loop_regex.findall(line)
        if len(is_begin) > 0:
            new_section = is_begin[0]
            lines_by_section[section_stack[-1]].append(list([new_section])) # used as insertion point later
            section_stack.append(new_section)
            lines_by_section[new_section] = []
        elif len(is_loop) > 0:
            new_section = is_loop[0]
            lines_by_section[section_stack[-1]].append(list([new_section])) # used as insertion point later
            section_stack.append(new_section)
            lines_by_section[new_section] = []
        elif len(is_end) > 0:
            closing_section = is_end[0]
            if (0 == len(section_stack)) or (section_stack[-1] != closing_section):
                raise RuntimeError("file %s, line %d: 'END:' of section '%s' is improperly nested; open-section stack is %s" \
                                   % (current_input, current_line, closing_section, str(section_stack)))
            section_stack.pop()
        elif len(is_loop_end) > 0:
            closing_section = is_loop_end[0]
            if (0 == len(section_stack)) or (section_stack[-1] != closing_section):
                raise RuntimeError("file %s, line %d: 'END-LOOP:' of loop '%s' is improperly nested; open-section stack is %s" \
                                   % (current_input, current_line, closing_section, str(section_stack)))
            section_stack.pop()
        else:
            # verbatim line
            lines_by_section[section_stack[-1]].append(line)
    if (len(section_stack) != 1) or (section_stack[0] != default_section):
        raise RuntimeError("file %s, line %d: unterminated statement; open-section stack is %s"
                           % (current_input, current_line, str(section_stack)))
    # build the root, which will recursively build other encountered sections and loops
    build_section(default_section, subst_kv)

src_dir = os.environ['RELEASE_NOTES_SRC']
dest_dir = os.environ['RELEASE_NOTES_DEST']
template_index = 'template-index.html'
template_index_path = path.join(src_dir, template_index)
template_version = 'template-version.html'
template_version_path = path.join(src_dir, template_version)
template_daily = 'template-daily.html'
template_daily_path = path.join(src_dir, template_daily)
current_input = "" # used when raising exceptions
current_line = 0 # used when raising exceptions

if __name__ == '__main__':
    # by default, all versions are returned; or, specify which ones
    if len(sys.argv) > 1: lineage = sys.argv[1:]
    else: lineage = version_lineage
    
    # read index page's template lines
    current_input = template_index_path
    index_in = file(current_input)
    index_lines = index_in.readlines()
    index_in.close()

    # compile all regular expressions once
    begin_section_regex = re.compile(r'BEGIN:([^ ]+) ')
    end_section_regex =     re.compile(r'END:([^ ]+) ')
    begin_loop_regex = re.compile(r'BEGIN-LOOP:([^ ]+) ')
    end_loop_regex =     re.compile(r'END-LOOP:([^ ]+) ')
    http_link_regex = re.compile(r'<(http://[^>]*)>')
    trac_link_regex = re.compile(r'\(Trac #(\d+)\)')
    
    # write an index file based on actual versions
    index_out = file(path.join(dest_dir, "index.html"), 'w')
    variables = {}
    generate_html(index_out, index_lines, variables, recent_versions=version_lineage)
    index_out.close()
    
    # read version page's template lines
    current_input = template_version_path
    version_in = file(current_input)
    version_lines = version_in.readlines()
    version_in.close()
    
    # write one file per release
    for i, version in enumerate(lineage):
        version_out = file(path.join(dest_dir, "%s.html" % version), 'w')
        variables = {'current_version': version, 'version': version}
        generate_html(version_out, version_lines, variables, recent_versions=lineage[:i])
        version_out.close()
    
    # read daily build page's template lines
    current_input = template_daily_path
    daily_in = file(current_input)
    daily_lines = daily_in.readlines()
    daily_in.close()
    
    # write a file for the daily build
    full_lineage = daily_build_lineage[:]
    full_lineage.extend(lineage)
    daily_out = file(path.join(dest_dir, "daily.html"), 'w')
    variables = {'current_version': daily_build_lineage[0]}
    generate_html(daily_out, daily_lines, variables, recent_versions=full_lineage)
    daily_out.close()

