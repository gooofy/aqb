#!/usr/bin/env python

import sys
import re

from marko import Markdown, block, inline, helpers

#OUTPUT_FN = "/home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/foo.guide"

def centerline(s):

    l = len(s)
    if l>80:
        return s
    return " " * (37-int(l/2)) + s

def mangleNode(s):

    t = ""

    for c in s:
        if c.isalpha():
            t = t + c
        else:
            t = t + "_"

    return t

def aguideEscape(s):
    return s.replace ('\\', '\\\\').replace ('@', '\\@')

class Document(block.Document):
    def __init__(self, text):
        self.tocnodes = []
        super().__init__(text)

class InternalLinkDef(block.BlockElement):

    #pattern = re.compile(r" {,3}\[\^([^\]]+)\]:[^\n\S]*(?=\S| {4})")
    #pattern = re.compile(r"\[([^\]]+)\]:[^\n\S]*(?=\S| {4})")
    pattern = re.compile(r"\[([^\]]+)\]:")
    priority = 6

    def __init__(self, match):
        self.label = match.group(1)
        self._prefix = re.escape(match.group())
        self._second_prefix = r" {1,4}"

    @classmethod
    def match(cls, source):
        return source.expect_re(cls.pattern)

    @classmethod
    def parse(cls, source):
        state = cls(source.match)
        with source.under_state(state):
            state.children = block.parser.parse(source)
        if not state.label == "MAIN":
            source.root.tocnodes.append(state)
        return state


class InternalLinkRef(inline.InlineElement):

    #pattern = re.compile(r"\[\^([^\]]+)\]")
    pattern = re.compile(r"\[([^\]]+)\](?=([^:(]|$))")
    priority = 6

    def __init__(self, match):
        self.label = match.group(1)

    @classmethod
    def find(cls, text):
        for match in super().find(text):
            yield match

class ExternalLinkRef(inline.InlineElement):

    pattern = re.compile(r"!?\[([^\]]+)\]\(([^)]+)\)")
    priority = 6

    def __init__(self, match):
        self.label = match.group(1)
        self.refdoc = match.group(2)

    @classmethod
    def find(cls, text):
        for match in super().find(text):
            # label = helpers.normalize_label(match.group(1))
            # if label in inline._root_node.footnotes:
            yield match

class TableOfContents(block.BlockElement):

    priority = 6
    pattern = re.compile(r"(:toc:)")
    inline_children = True

    def __init__(self, match):  # type: (Match) -> None
        self.level = len(match.group(1))
        self.children = ""

    @classmethod
    def match(cls, source):  # type: (Source) -> Optional[Match]
        return source.expect_re(cls.pattern)

    @classmethod
    def parse(cls, source):  # type: (Source) -> Optional[Match]
        m = source.match
        source.consume()
        return m


class AmigaGuideMixin(object):

    def __init__(self):
        self.indent = 0
        self.inNode = False

    def render_internal_link_ref(self, element):
        return '@{"%s" link "%s"}' % (element.label, mangleNode(element.label))

    def render_internal_link_def(self, element):
        prf = "\n@endnode\n" if self.inNode else "\n"

        self.inNode = True

        return prf + '@node %s "%s"\n' % (mangleNode(element.label), element.label)

    def render_external_link_ref(self, element):
        if "http" in element.refdoc:
            return ""
        return '@{"%s" link "%s/main"}' % (element.label, element.refdoc.replace('.md', '.guide'))

    def render_link(self, element):
        return element.dest

    def render_paragraph(self, element):
        children = self.render_children(element)
        if element._tight:
            return children
        else:
            return f"\n{children}\n"

    def render_heading(self, element):

        if element.level == 1:
            b = self.render_children(element)
            return "\n\n@{b}%s@{ub}\n\n" % centerline(b)
        elif element.level == 2:
            return "\n\n@{b}%s@{ub}\n" % self.render_children(element)
        elif element.level == 3:
            return "\n\n@{b}%s@{ub}\n" % self.render_children(element)
        else:
            return "\n\n" + self.render_children(element) + "\n"

    def render_list(self, element):
        # FIXME: if element.ordered:
        self.indent += 1
        b = self.render_children(element)
        self.indent -= 1
        return b

    def render_list_item(self, element):
        return "\n" + "  " * self.indent + "* " + self.render_children(element)

    def render_fenced_code(self, element):
        b = '\n' + element.children[0].children
        lines = b.split('\n')
        return aguideEscape('\n  '.join(lines)+'\n')

    def render_code_span(self, element):
        return aguideEscape(element.children)

    def render_table_of_contents(self, element):

        lines = "\n"

        for n in self.root_node.tocnodes:
            lines = lines + '@{"%s" link "%s"}\n' % (n.label, mangleNode(n.label))

        return lines

    def render_children(self, element):

        rendered = [self.render(child) for child in element.children]

        body = "".join(rendered)

        if element is self.root_node:
            suff = "@endnode\n" if self.inNode else ""
            return "@DATABASE\n" + body + suff
        else:
            return body

class AmigaGuide:
    elements = [InternalLinkDef, InternalLinkRef, ExternalLinkRef, Document, TableOfContents]
    renderer_mixins = [AmigaGuideMixin]


markdown = Markdown(extensions=[AmigaGuide])

if len(sys.argv) != 2:
    print ("usage: %s <foo.md>" % sys.argv[0])
    sys.exit(1)

infn = sys.argv[1]

with open (infn, "r") as mdf:
    md = mdf.read()

aguide = markdown.convert(md)

#with open (OUTPUT_FN, "w", encoding="latin1") as outf:
#    outf.write(aguide)

print (aguide)

