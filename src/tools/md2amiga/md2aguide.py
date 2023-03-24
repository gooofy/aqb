#!/usr/bin/env python3

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
            t = t + c.lower()
        elif c==' ':
            t = t + "-"
        elif c=='(':
            t = t + "("
        elif c==')':
            t = t + ")"
        elif c=='#':
            t = t + "#"

    return t

def aguideEscape(s):
    return s.replace('\\_', '_').replace('\\#', '#').replace ('\\', '\\\\').replace('@', '\\@')

class Document(block.Document):
    def __init__(self, text):
        self.tocnodes = []
        super().__init__(text)

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

class HeadingTOC(block.Heading):
    override = True

    @classmethod
    def parse(cls, source):  # type: (Source) -> Optional[Match]
        m = source.match
        level = len(m.group(1))
        if level == 2:
            source.root.tocnodes.append(m.group(2).strip())
        source.consume()
        return m


class AmigaGuideMixin(object):

    def __init__(self):
        self.indent = 0
        self.inNode = True

    def render_external_link_ref(self, element):
        if "http" in element.refdoc:
            return ""
        if '#' in element.refdoc:
            return '@{"%s" link "%s"}' % (element.label, element.refdoc[1:])

        return '@{"%s" link "%s/main"}' % (element.label, element.refdoc.replace('.md', '.guide'))

    def render_link(self, element):
        return element.dest

    def render_paragraph(self, element):
        children = self.render_children(element)
        if element._tight:
            return children
        else:
            return f"\n{children}\n"

    def _make_node (self, label):
        if not self.inNode:
            self.inNode = True
            return ''
        prf = "\n@endnode\n"
        self.inNode = True
        return prf + '@node %s "%s"\n' % (mangleNode(label), label)

    def render_heading(self, element):

        if element.level == 1:
            b = self.render_children(element)
            return "\n\n@{b}%s@{ub}\n" % centerline(b)
        elif element.level == 2:
            b = self.render_children(element)
            return self._make_node (b) + "\n\n@{b}%s@{ub}\n" % b
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

        for label in self.root_node.tocnodes:
            lines = lines + '@{"%s" link "%s"}\n' % (aguideEscape(label), mangleNode(label))

        return lines

    def render_children(self, element):

        rendered = [self.render(child) for child in element.children]

        body = "".join(rendered)

        if element is self.root_node:

            suff = ("\n"
                    "@endnode\n"
                    "@node navidx \"Index\"\n"
                    "\n\n"
                    " * @{\"Start\" link \"README.guide/main\"}\n\n"
                    " * @{\"Reference: Core\" link \"help/RefCore.guide/main\"}\n"
                    " * @{\"Reference: Amiga specific commands\" link \"help/RefAmiga.guide/main\"}\n"
                    " * @{\"Reference: IFFSupport module\" link \"help/IFFSupport.guide/main\"}\n"
                    " * @{\"Reference: AnimSupport module\" link \"help/AnimSupport.guide/main\"}\n"
                    "@endnode\n"
                    "@index navidx\n")
            return "@DATABASE\n@node MAIN\n" + body + suff
        else:
            return body

    def render_plain_text(self, element):
        if isinstance(element.children, str):
            return aguideEscape(element.children)
        return self.render_children(element)

    def render_raw_text(self, element):
        return aguideEscape(element.children)

class AmigaGuide:
    elements = [HeadingTOC, ExternalLinkRef, Document, TableOfContents]
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

