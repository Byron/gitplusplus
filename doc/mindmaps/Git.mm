<map version="0.9.0">
<!-- To view this file, download free mind mapping software FreeMind from http://freemind.sourceforge.net -->
<node COLOR="#000000" CREATED="1301408977165" ID="ID_1022226842" MODIFIED="1302079132380" TEXT="Git">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      &lt;Scratch&gt;
    </p>
    <p>
      Object:
    </p>
    <p>
      working_tree_dir
    </p>
    <p>
      rev_parse()
    </p>
    <p>
      
    </p>
    <p>
      git_dir
    </p>
    <p>
      info()
    </p>
    <p>
      stream()
    </p>
    <p>
      
    </p>
    <p>
      Reference
    </p>
    <p>
      rev_parse()
    </p>
    <p>
      
    </p>
    <p>
      git_dir
    </p>
    <p>
      
    </p>
    <p>
      ref_repo:
    </p>
    <p>
      refs
    </p>
    <p>
      heads
    </p>
    <p>
      head
    </p>
    <p>
      tags
    </p>
  </body>
</html>
</richcontent>
<font NAME="SansSerif" SIZE="20"/>
<hook NAME="accessories/plugins/AutomaticLayout.properties"/>
<node COLOR="#0033ff" CREATED="1301409048635" HGAP="33" ID="ID_1102189277" MODIFIED="1301516806091" POSITION="right" TEXT="Transport" VSHIFT="-5">
<edge STYLE="sharp_bezier" WIDTH="8"/>
<font NAME="SansSerif" SIZE="18"/>
<node COLOR="#00b439" CREATED="1301419263829" HGAP="21" ID="ID_1378015452" MODIFIED="1301600501015" TEXT="Git Protocol" VSHIFT="-22">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      See git/Documentation/technical/pack-protocol.txt
    </p>
    <p>
      &#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;/protocol-common.txt
    </p>
    <p>
      &#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;/protocol-capabilities
    </p>
    <p>
      
    </p>
    <p>
      Talk to each other in packet lines : 4 bytes hex encoded size of the following line, which includes a newline, e.g. 0006a\n
    </p>
    <p>
      communicate available refs and capabilities
    </p>
    <p>
      Communicate required capabilities (by the receiving end)
    </p>
  </body>
</html></richcontent>
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
<node COLOR="#990000" CREATED="1301507487480" ID="ID_1996084795" MODIFIED="1301507494936" TEXT="push">
<font NAME="SansSerif" SIZE="14"/>
<node COLOR="#111111" CREATED="1301507444239" HGAP="16" ID="ID_1137749226" MODIFIED="1301515890332" TEXT="git-send-pack -&gt; git-receive-pack" VSHIFT="-10">
<font NAME="SansSerif" SIZE="12"/>
</node>
</node>
<node COLOR="#990000" CREATED="1301507467648" ID="ID_1004870318" MODIFIED="1301507469899" TEXT="fetch">
<font NAME="SansSerif" SIZE="14"/>
<node COLOR="#111111" CREATED="1301507453247" HGAP="19" ID="ID_1091853630" MODIFIED="1301515883932" TEXT="git-fetch-pack -&gt; git-upload-pack" VSHIFT="6">
<font NAME="SansSerif" SIZE="12"/>
</node>
</node>
</node>
<node COLOR="#00b439" CREATED="1301515540629" ID="ID_1803212886" MODIFIED="1301518944840" TEXT="Custom Protocols">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      Implemented via git-remote-&lt;name&gt; helpers, which are called according to the protocol, e.g. myhelper://hello/world.git. A helper can also be enforced using the remote.&lt;name&gt;.vcs option.
    </p>
    <p>
      More information in http://www.kernel.org/pub/software/scm/git/docs/git-remote-helpers.html
    </p>
    <p>
      
    </p>
    <p>
      The only disadvantage is that it calls a separate process. Depending on how far we go, we could just implement everything on our own, but it should be enough to just send a pack to a waiting git-receive-pack. Unfortunately, in the case of a helper, we would have to start the process ourselves.
    </p>
  </body>
</html></richcontent>
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
<icon BUILTIN="ksmiletris"/>
</node>
</node>
<node COLOR="#0033ff" CREATED="1301409053281" HGAP="32" ID="ID_1718210506" MODIFIED="1301515903651" POSITION="right" TEXT="Pack" VSHIFT="31">
<edge STYLE="sharp_bezier" WIDTH="8"/>
<font NAME="SansSerif" SIZE="18"/>
<node COLOR="#00b439" CREATED="1301409069144" ID="ID_361792066" MODIFIED="1301409215214" TEXT="Read">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
</node>
<node COLOR="#00b439" CREATED="1301409072272" ID="ID_1605663403" MODIFIED="1301520623420" TEXT="Write">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      How git writes packs
    </p>
  </body>
</html></richcontent>
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
<node COLOR="#990000" CREATED="1301427466524" ID="ID_211737654" MODIFIED="1301487236530" TEXT="Thin">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      May relate to an object which is existing on the target side. This also means that the pack itself will be need additional objects to be complete, or else it looks up ref deltas in the existing object database.
    </p>
  </body>
</html></richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
<node COLOR="#990000" CREATED="1301427469916" ID="ID_97454794" MODIFIED="1301519538406" TEXT="Default">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      Packs all required objects, using the newest one as a base for deltas, as required. This version is only used for offline packs. When they are sent to over the wire, thin packs are used instead which is usually what we want when sending over the internet.
    </p>
  </body>
</html></richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
</node>
<node COLOR="#00b439" CREATED="1301409082681" ID="ID_402064347" MODIFIED="1301409215216" TEXT="Index">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
<node COLOR="#990000" CREATED="1301409342262" ID="ID_863735749" MODIFIED="1301520780015" TEXT="Version 1">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      Obsolete
    </p>
  </body>
</html></richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
<node COLOR="#990000" CREATED="1301409349590" ID="ID_1994367983" MODIFIED="1301520779798" TEXT="Version 2">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      Version 2
    </p>
    <p>
      &#160;- uses arrays for offsets, crcs and sha1 hashes
    </p>
    <p>
      &#160;- provides crcs for compressed streams for quick integrity checking
    </p>
  </body>
</html></richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
<node COLOR="#990000" CREATED="1301409452648" ID="ID_4712274" MODIFIED="1301520779344" TEXT="Version 9">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      Special purpose index keeping optimized for quick rewrites. It includes a path index that can be used to find previous revisions by path.
    </p>
    <p>
      
    </p>
    <p>
      All data is stored in a default pack file, but without a changing name based on the sha1 of shas of all included objects. This allows that a delta can be created in the moment we are adding the new revision, which safes space an speed as it will take much less time to compress a delta.
    </p>
  </body>
</html></richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
</node>
</node>
<node COLOR="#0033ff" CREATED="1301409089745" HGAP="50" ID="ID_327615476" MODIFIED="1301515901390" POSITION="left" TEXT="Performance" VSHIFT="-5">
<edge STYLE="sharp_bezier" WIDTH="8"/>
<font NAME="SansSerif" SIZE="18"/>
<node COLOR="#00b439" CREATED="1301409096113" ID="ID_978183245" MODIFIED="1301409215219" TEXT="Loose Objects">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
</node>
<node COLOR="#00b439" CREATED="1301409101601" ID="ID_580832331" MODIFIED="1301409215220" TEXT="Delta-Diffing">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
</node>
</node>
</node>
</map>
