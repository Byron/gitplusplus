<map version="0.9.0">
<!-- To view this file, download free mind mapping software FreeMind from http://freemind.sourceforge.net -->
<node COLOR="#000000" CREATED="1301408977165" ID="ID_1022226842" MODIFIED="1301409335347" TEXT="Git">
<font NAME="SansSerif" SIZE="20"/>
<hook NAME="accessories/plugins/AutomaticLayout.properties"/>
<node COLOR="#0033ff" CREATED="1301409048635" ID="ID_1102189277" MODIFIED="1301409215212" POSITION="right" TEXT="Transport">
<edge STYLE="sharp_bezier" WIDTH="8"/>
<font NAME="SansSerif" SIZE="18"/>
<node COLOR="#00b439" CREATED="1301419263829" ID="ID_1378015452" MODIFIED="1301419269006" TEXT="Git Protocol">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
</node>
</node>
<node COLOR="#0033ff" CREATED="1301409053281" HGAP="33" ID="ID_1718210506" MODIFIED="1301409338658" POSITION="right" TEXT="Pack" VSHIFT="21">
<edge STYLE="sharp_bezier" WIDTH="8"/>
<font NAME="SansSerif" SIZE="18"/>
<node COLOR="#00b439" CREATED="1301409069144" ID="ID_361792066" MODIFIED="1301409215214" TEXT="Read">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
</node>
<node COLOR="#00b439" CREATED="1301409072272" ID="ID_1605663403" MODIFIED="1301409294349" TEXT="Write">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
</node>
<node COLOR="#00b439" CREATED="1301409082681" ID="ID_402064347" MODIFIED="1301409215216" TEXT="Index">
<edge STYLE="bezier" WIDTH="thin"/>
<font NAME="SansSerif" SIZE="16"/>
<node COLOR="#990000" CREATED="1301409342262" ID="ID_863735749" MODIFIED="1301419259848" TEXT="Version 1">
<richcontent TYPE="NOTE"><html>
  <head>
    
  </head>
  <body>
    <p>
      Obsolete
    </p>
  </body>
</html>
</richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
<node COLOR="#990000" CREATED="1301409349590" ID="ID_1994367983" MODIFIED="1301419259519" TEXT="Version 2">
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
</html>
</richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
<node COLOR="#990000" CREATED="1301409452648" ID="ID_4712274" MODIFIED="1301411410529" TEXT="Version 9">
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
      All data is stored in a default pack file, but without a changing name based on the sha1 of shas of all included objects. This assure
    </p>
  </body>
</html>
</richcontent>
<font NAME="SansSerif" SIZE="14"/>
</node>
</node>
</node>
<node COLOR="#0033ff" CREATED="1301409089745" HGAP="39" ID="ID_327615476" MODIFIED="1301409336497" POSITION="left" TEXT="Performance" VSHIFT="-6">
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
