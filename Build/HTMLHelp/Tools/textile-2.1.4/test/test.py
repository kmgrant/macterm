import textile
import re
import unittest
from nose.tools import eq_, assert_true


"""
('>>> import textile')
'<p>&#62;&#62;&#62; import textile</p>'

"""

class TestKnownValues():
    xhtml_known_values = (
        ('hello, world', '\t<p>hello, world</p>'),

        ('A single paragraph.\n\nFollowed by another.','\t<p>A single paragraph.</p>\n\n\t<p>Followed by another.</p>'),

        ('I am <b>very</b> serious.\n\n<pre>\nI am <b>very</b> serious.\n</pre>',
         '\t<p>I am <b>very</b> serious.</p>\n\n<pre>\nI am &#60;b&#62;very&#60;/b&#62; serious.\n</pre>'),

        ('I spoke.\nAnd none replied.', '\t<p>I spoke.<br />And none replied.</p>'),

        ('"Observe!"', '\t<p>&#8220;Observe!&#8221; </p>'),

        ('Observe -- very nice!', '\t<p>Observe &#8212; very nice!</p>'),

        ('Observe - tiny and brief.', '\t<p>Observe &#8211; tiny and brief.</p>'),

        ('Observe...', '\t<p>Observe&#8230;</p>'),

        ('Observe ...', '\t<p>Observe &#8230;</p>'),

        ('Observe: 2 x 2.', '\t<p>Observe: 2 &#215; 2.</p>'),

        ('one(TM), two(R), three(C).', '\t<p>one&#8482;, two&#174;, three&#169;.</p>'),

        ('h1. Header 1', '\t<h1>Header 1</h1>'),

        ('h2. Header 2', '\t<h2>Header 2</h2>'),

        ('h3. Header 3', '\t<h3>Header 3</h3>'),

        ('An old text\n\nbq. A block quotation.\n\nAny old text''',
        '\t<p>An old text</p>\n\n\t<blockquote>\n\t\t<p>A block quotation.</p>\n\t</blockquote>\n\n\t<p>Any old text</p>'),

        ('I _believe_ every word.', '\t<p>I <em>believe</em> every word.</p>'),

        ('And then? She *fell*!', '\t<p>And then? She <strong>fell</strong>!</p>'),

        ('I __know__.\nI **really** __know__.', '\t<p>I <i>know</i>.<br />I <b>really</b> <i>know</i>.</p>'),

        ("??Cat's Cradle?? by Vonnegut", '\t<p><cite>Cat&#8217;s Cradle</cite> by Vonnegut</p>'),

        ('Convert with @str(foo)@', '\t<p>Convert with <code>str(foo)</code></p>'),

        ('I\'m -sure- not sure.', '\t<p>I&#8217;m <del>sure</del> not sure.</p>'),

        ('You are a +pleasant+ child.', '\t<p>You are a <ins>pleasant</ins> child.</p>'),

        ('a ^2^ + b ^2^ = c ^2^', '\t<p>a <sup>2</sup> + b <sup>2</sup> = c <sup>2</sup></p>'),

        ('log ~2~ x', '\t<p>log <sub>2</sub> x</p>'),

        ('I\'m %unaware% of most soft drinks.', '\t<p>I&#8217;m <span>unaware</span> of most soft drinks.</p>'),

        ("I'm %{color:red}unaware%\nof most soft drinks.", '\t<p>I&#8217;m <span style="color:red;">unaware</span><br />of most soft drinks.</p>'),

        ('p(example1). An example', '\t<p class="example1">An example</p>'),

        ('p(#big-red). Red here', '\t<p id="big-red">Red here</p>'),

        ('p(example1#big-red2). Red here', '\t<p class="example1" id="big-red2">Red here</p>'),

        ('p{color:blue;margin:30px}. Spacey blue', '\t<p style="color:blue;margin:30px;">Spacey blue</p>'),

        ('p[fr]. rouge', '\t<p lang="fr">rouge</p>'),

        ('I seriously *{color:red}blushed*\nwhen I _(big)sprouted_ that\ncorn stalk from my\n%[es]cabeza%.',
        '\t<p>I seriously <strong style="color:red;">blushed</strong><br />when I <em class="big">sprouted</em>'
        ' that<br />corn stalk from my<br /><span lang="es">cabeza</span>.</p>'),

        ('p<. align left', '\t<p style="text-align:left;">align left</p>'),

        ('p>. align right', '\t<p style="text-align:right;">align right</p>'),

        ('p=. centered', '\t<p style="text-align:center;">centered</p>'),

        ('p<>. justified', '\t<p style="text-align:justify;">justified</p>'),

        ('p(. left ident 1em', '\t<p style="padding-left:1em;">left ident 1em</p>'),

        ('p((. left ident 2em', '\t<p style="padding-left:2em;">left ident 2em</p>'),

        ('p))). right ident 3em', '\t<p style="padding-right:3em;">right ident 3em</p>'),

        ('h2()>. Bingo.', '\t<h2 style="padding-left:1em;padding-right:1em;text-align:right;">Bingo.</h2>'),

        ('h3()>[no]{color:red}. Bingo', '\t<h3 style="color:red;padding-left:1em;padding-right:1em;text-align:right;" lang="no">Bingo</h3>'),

        ('<pre>\n<code>\na.gsub!( /</, "" )\n</code>\n</pre>',
         '<pre>\n<code>\na.gsub!( /&#60;/, &#34;&#34; )\n</code>\n</pre>'),

        ('<div style="float:right;">\n\nh3. Sidebar\n\n"Hobix":http://hobix.com/\n"Ruby":http://ruby-lang.org/\n\n</div>\n\n'
         'The main text of the\npage goes here and will\nstay to the left of the\nsidebar.',
         '\t<p><div style="float:right;"></p>\n\n\t<h3>Sidebar</h3>\n\n\t<p><a href="http://hobix.com/">Hobix</a><br />'
         '<a href="http://ruby-lang.org/">Ruby</a></p>\n\n\t<p></div></p>\n\n\t<p>The main text of the<br />'
         'page goes here and will<br />stay to the left of the<br />sidebar.</p>'),

        ('# A first item\n# A second item\n# A third',
         '\t<ol>\n\t\t<li>A first item</li>\n\t\t<li>A second item</li>\n\t\t<li>A third</li>\n\t</ol>'),

        ('# Fuel could be:\n## Coal\n## Gasoline\n## Electricity\n# Humans need only:\n## Water\n## Protein',
         '\t<ol>\n\t\t<li>Fuel could be:\n\t<ol>\n\t\t<li>Coal</li>\n\t\t<li>Gasoline</li>\n\t\t<li>Electricity</li>\n\t</ol></li>\n\t\t'
         '<li>Humans need only:\n\t<ol>\n\t\t<li>Water</li>\n\t\t<li>Protein</li>\n\t</ol></li>\n\t</ol>'),

        ('* A first item\n* A second item\n* A third',
         '\t<ul>\n\t\t<li>A first item</li>\n\t\t<li>A second item</li>\n\t\t<li>A third</li>\n\t</ul>'),

        ('* Fuel could be:\n** Coal\n** Gasoline\n** Electricity\n* Humans need only:\n** Water\n** Protein',
         '\t<ul>\n\t\t<li>Fuel could be:\n\t<ul>\n\t\t<li>Coal</li>\n\t\t<li>Gasoline</li>\n\t\t<li>Electricity</li>\n\t</ul></li>\n\t\t'
         '<li>Humans need only:\n\t<ul>\n\t\t<li>Water</li>\n\t\t<li>Protein</li>\n\t</ul></li>\n\t</ul>'),

        ('I searched "Google":http://google.com.', '\t<p>I searched <a href="http://google.com">Google</a>.</p>'),

        ('I am crazy about "Hobix":hobix\nand "it\'s":hobix "all":hobix I ever\n"link to":hobix!\n\n[hobix]http://hobix.com',
         '\t<p>I am crazy about <a href="http://hobix.com">Hobix</a><br />and <a href="http://hobix.com">it&#8217;s</a> '
         '<a href="http://hobix.com">all</a> I ever<br /><a href="http://hobix.com">link to</a>!</p>\n\n'),

        ('!http://hobix.com/sample.jpg!', '\t<p><img src="http://hobix.com/sample.jpg" alt="" /></p>'),

        ('!openwindow1.gif(Bunny.)!', '\t<p><img src="openwindow1.gif" title="Bunny." alt="Bunny." /></p>'),

        ('!openwindow1.gif!:http://hobix.com/', '\t<p><a href="http://hobix.com/" class="img"><img src="openwindow1.gif" alt="" /></a></p>'),

        ('!>obake.gif!\n\nAnd others sat all round the small\nmachine and paid it to sing to them.',
         '\t<p><img src="obake.gif" style="text-align:right;" alt="" /></p>\n\n\t'
         '<p>And others sat all round the small<br />machine and paid it to sing to them.</p>'),

        ('We use CSS(Cascading Style Sheets).', '\t<p>We use <acronym title="Cascading Style Sheets">CSS</acronym>.</p>'),

        ('|one|two|three|\n|a|b|c|',
         '\t<table>\n\t\t<tr>\n\t\t\t<td>one</td>\n\t\t\t<td>two</td>\n\t\t\t<td>three</td>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td>a</td>\n\t\t\t<td>b</td>\n\t\t\t<td>c</td>\n\t\t</tr>\n\t</table>'),

        ('| name | age | sex |\n| joan | 24 | f |\n| archie | 29 | m |\n| bella | 45 | f |',
         '\t<table>\n\t\t<tr>\n\t\t\t<td> name </td>\n\t\t\t<td> age </td>\n\t\t\t<td> sex </td>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td> joan </td>\n\t\t\t<td> 24 </td>\n\t\t\t<td> f </td>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td> archie </td>\n\t\t\t<td> 29 </td>\n\t\t\t<td> m </td>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td> bella </td>\n\t\t\t<td> 45 </td>\n\t\t\t<td> f </td>\n\t\t</tr>\n\t</table>'),

        ('|_. name |_. age |_. sex |\n| joan | 24 | f |\n| archie | 29 | m |\n| bella | 45 | f |',
         '\t<table>\n\t\t<tr>\n\t\t\t<th>name </th>\n\t\t\t<th>age </th>\n\t\t\t<th>sex </th>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td> joan </td>\n\t\t\t<td> 24 </td>\n\t\t\t<td> f </td>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td> archie </td>\n\t\t\t<td> 29 </td>\n\t\t\t<td> m </td>\n\t\t</tr>'
         '\n\t\t<tr>\n\t\t\t<td> bella </td>\n\t\t\t<td> 45 </td>\n\t\t\t<td> f </td>\n\t\t</tr>\n\t</table>'),

        # ('<script>alert("hello");</script>', ''),

        ('pre.. Hello\n\nHello Again\n\np. normal text', '<pre>Hello\n\nHello Again\n</pre>\n\n\t<p>normal text</p>'),

        ('<pre>this is in a pre tag</pre>', '<pre>this is in a pre tag</pre>'),

        ('"test1":http://foo.com/bar--baz\n\n"test2":http://foo.com/bar---baz\n\n"test3":http://foo.com/bar-17-18-baz', 
         '\t<p><a href="http://foo.com/bar--baz">test1</a></p>\n\n\t'
         '<p><a href="http://foo.com/bar---baz">test2</a></p>\n\n\t'
         '<p><a href="http://foo.com/bar-17-18-baz">test3</a></p>'),

        # ('"foo ==(bar)==":#foobar', '\t<p><a href="#foobar">foo (bar)</a></p>'),

        ('!http://render.mathim.com/A%5EtAx%20%3D%20A%5Et%28Ax%29.!', 
         '\t<p><img src="http://render.mathim.com/A%5EtAx%20%3D%20A%5Et%28Ax%29." alt="" /></p>'),

        ('* Point one\n* Point two\n## Step 1\n## Step 2\n## Step 3\n* Point three\n** Sub point 1\n** Sub point 2',
         '\t<ul>\n\t\t<li>Point one</li>\n\t\t<li>Point two\n\t<ol>\n\t\t<li>Step 1</li>\n\t\t<li>Step 2</li>\n\t\t'
         '<li>Step 3</li>\n\t</ol></li>\n\t\t<li>Point three\n\t<ul>\n\t\t<li>Sub point 1</li>\n\t\t'
         '<li>Sub point 2</li>\n\t</ul></li>\n\t</ul>'),

        ('@array[4] = 8@', '\t<p><code>array[4] = 8</code></p>'),

        ('#{color:blue} one\n# two\n# three', 
         '\t<ol style="color:blue;">\n\t\t<li>one</li>\n\t\t<li>two</li>\n\t\t<li>three</li>\n\t</ol>'),

        ('Links (like "this":http://foo.com), are now mangled in 2.1.0, whereas 2.0 parsed them correctly.',
         '\t<p>Links (like <a href="http://foo.com">this</a>), are now mangled in 2.1.0, whereas 2.0 parsed them correctly.</p>'),

        ('@monospaced text@, followed by text', 
         '\t<p><code>monospaced text</code>, followed by text</p>'),

        ('h2. A header\n\n\n\n\n\nsome text', '\t<h2>A header</h2>\n\n\t<p>some text</p>'),

    )

    # A few extra cases for HTML4
    html_known_values = (
        ('I spoke.\nAnd none replied.', '\t<p>I spoke.<br>And none replied.</p>'),
        ('I __know__.\nI **really** __know__.', '\t<p>I <i>know</i>.<br>I <b>really</b> <i>know</i>.</p>'),
        ("I'm %{color:red}unaware%\nof most soft drinks.", '\t<p>I&#8217;m <span style="color:red;">unaware</span><br>of most soft drinks.</p>'),
        ('I seriously *{color:red}blushed*\nwhen I _(big)sprouted_ that\ncorn stalk from my\n%[es]cabeza%.',
        '\t<p>I seriously <strong style="color:red;">blushed</strong><br>when I <em class="big">sprouted</em>'
        ' that<br>corn stalk from my<br><span lang="es">cabeza</span>.</p>'),
        ('<pre>\n<code>\na.gsub!( /</, "" )\n</code>\n</pre>',
         '<pre>\n<code>\na.gsub!( /&#60;/, &#34;&#34; )\n</code>\n</pre>'),
        ('<div style="float:right;">\n\nh3. Sidebar\n\n"Hobix":http://hobix.com/\n"Ruby":http://ruby-lang.org/\n\n</div>\n\n'
         'The main text of the\npage goes here and will\nstay to the left of the\nsidebar.',
         '\t<p><div style="float:right;"></p>\n\n\t<h3>Sidebar</h3>\n\n\t<p><a href="http://hobix.com/">Hobix</a><br>'
         '<a href="http://ruby-lang.org/">Ruby</a></p>\n\n\t<p></div></p>\n\n\t<p>The main text of the<br>'
         'page goes here and will<br>stay to the left of the<br>sidebar.</p>'),
        ('I am crazy about "Hobix":hobix\nand "it\'s":hobix "all":hobix I ever\n"link to":hobix!\n\n[hobix]http://hobix.com',
         '\t<p>I am crazy about <a href="http://hobix.com">Hobix</a><br>and <a href="http://hobix.com">it&#8217;s</a> '
         '<a href="http://hobix.com">all</a> I ever<br><a href="http://hobix.com">link to</a>!</p>\n\n'),
        ('!http://hobix.com/sample.jpg!', '\t<p><img src="http://hobix.com/sample.jpg" alt=""></p>'),
        ('!openwindow1.gif(Bunny.)!', '\t<p><img src="openwindow1.gif" title="Bunny." alt="Bunny."></p>'),
        ('!openwindow1.gif!:http://hobix.com/', '\t<p><a href="http://hobix.com/" class="img"><img src="openwindow1.gif" alt=""></a></p>'),
        ('!>obake.gif!\n\nAnd others sat all round the small\nmachine and paid it to sing to them.',
         '\t<p><img src="obake.gif" style="text-align:right;" alt=""></p>\n\n\t'
         '<p>And others sat all round the small<br>machine and paid it to sing to them.</p>'),
        ('!http://render.mathim.com/A%5EtAx%20%3D%20A%5Et%28Ax%29.!', 
         '\t<p><img src="http://render.mathim.com/A%5EtAx%20%3D%20A%5Et%28Ax%29." alt=""></p>'),
        ('notextile. <b> foo bar baz</b>\n\np. quux\n','<b> foo bar baz</b>\n\n\t<p>quux</p>')
    )

    def testKnownValuesXHTML(self):
        # XHTML
        for t, h in self.xhtml_known_values:
            yield self.check_textile, t, h, 'xhtml'

    def testKnownValuesHTML(self):
        # HTML4
        for t, h in self.html_known_values:
            yield self.check_textile, t, h, 'html'

    def check_textile(self, input, expected_output, html_type):
        output = textile.textile(input, html_type=html_type)
        eq_(output, expected_output)


class Tests():
    def testFootnoteReference(self):
        html = textile.textile('This is covered elsewhere[1].')
        assert_true(re.search('^\t<p>This is covered elsewhere<sup class="footnote"><a href="#fn[a-z0-9-]+">1</a></sup>.</p>$', html))
        html = textile.textile('YACC[1]')
        assert_true(re.search('^\t<p>YACC<sup class="footnote"><a href="#fn[a-z0-9-]+">1</a></sup></p>', html))

    def testFootnote(self):
        html = textile.textile('fn1. Down here, in fact.')
        assert_true(re.search('^\t<p id="fn[a-z0-9-]+" class="footnote"><sup>1</sup>Down here, in fact.</p>$', html))

    def testURLWithHyphens(self):
        eq_(textile.textile('"foo":http://google.com/one--two'), '\t<p><a href="http://google.com/one--two">foo</a></p>')

    def testIssue024TableColspan(self):
        eq_(textile.textile('|\\2. spans two cols |\n| col 1 | col 2 |'), 
            '\t<table>\n\t\t<tr>\n\t\t\t<td colspan="2">spans two cols </td>\n\t\t</tr>\n\t\t<tr>\n\t\t\t<td> col 1 </td>\n\t\t\t<td> col 2 </td>\n\t\t</tr>\n\t</table>')

    def testPBAColspan(self):
        eq_(textile.Textile().pba(r'\3', element='td'), ' colspan="3"')

    def testIssue002Escaping(self):
        foo = '"foo ==(bar)==":#foobar'
        eq_(textile.textile(foo), '\t<p><a href="#foobar">foo (bar)</a></p>')

    def testIssue014NewlinesInExtendedPreBlocks(self):
        text = "pre.. Hello\n\nAgain\n\np. normal text"        
        eq_(textile.textile(text), '<pre>Hello\n\nAgain\n</pre>\n\n\t<p>normal text</p>')

    def testURLWithParens(self):
        text = '"python":http://en.wikipedia.org/wiki/Python_(programming_language)'
        expect='\t<p><a href="http://en.wikipedia.org/wiki/Python_(programming_language)">python</a></p>'
        result=textile.textile(text)
        eq_(result, expect)


    def testTableWithHyphenStyles(self):
        text = 'table(linkblog-thumbnail).\n|(linkblog-thumbnail-cell). apple|bear|'
        expect = '\t<table class="linkblog-thumbnail">\n\t\t<tr>\n\t\t\t<td style="vertical-align:middle;" class="linkblog-thumbnail-cell">apple</td>\n\t\t\t<td>bear</td>\n\t\t</tr>\n\t</table>'
        result = textile.textile(text)
        eq_(result, expect)

    def testHeadOffset(self):
        text = 'h2. This is a header'
        head_offset = 2
        expect = '\t<h4>This is a header</h4>'
        result = textile.textile(text, head_offset=head_offset)
        eq_(result, expect)

    def TestIssue035(self):
        result = textile.textile('"z"')
        expect = '\t<p>&#8220;z&#8221; </p>'
        eq_(result, expect)

        result = textile.textile('" z"')
        expect = '\t<p>&#8220; z&#8221; </p>'
        eq_(result, expect)

    def TestIssue032(self):
        text = "|thing|||otherthing|"
        result = textile.textile(text)
        expect = "\t<table>\n\t\t<tr>\n\t\t\t<td>thing</td>\n\t\t\t<td></td>\n\t\t\t<td></td>\n\t\t\t<td>otherthing</td>\n\t\t</tr>\n\t</table>"
        eq_(result, expect)

    def TestIssue036(self):
        test = '"signup":signup\n[signup]http://myservice.com/signup'
        result = textile.textile(test)
        expect = '\t<p><a href="http://myservice.com/signup">signup</a></p>'
        eq_(result, expect)

        test = '"signup":signup\n[signup]https://myservice.com/signup'
        result = textile.textile(test)
        expect = '\t<p><a href="https://myservice.com/signup">signup</a></p>'
        eq_(result, expect)

    def TestNestedFormatting(self):
        test = "*_test text_*"
        result = textile.textile(test)
        expect = "\t<p><strong><em>test text</em></strong></p>"
        
        eq_(result, expect)


        test = "_*test text*_"
        result = textile.textile(test)
        expect = "\t<p><em><strong>test text</strong></em></p>"
        
        eq_(result, expect)

    def TestRestricted(self):
        test = "this is \"some\" *bold text*."
        result = textile.textile_restricted(test)
        expect = "\t<p>this is &#8220;some&#8221; <strong>bold text</strong>.</p>"
        
        eq_(result, expect)
        
        #Note that the HTML is escaped, thus rendering
        #the <script> tag harmless.
        test = "Here is some text.\n<script>alert('hello world')</script>"
        result = textile.textile_restricted(test)
        expect = "\t<p>Here is some text.<br />&#60;script&#62;alert('hello world&#8217;)&#60;/script&#62;</p>"
        
        eq_(result, expect)
