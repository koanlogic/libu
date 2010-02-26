<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns="http://www.w3.org/1999/xhtml"
  xmlns:str="http://exslt.org/strings"
>
  <xsl:output 
    method="xml"
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"
    doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN"
    indent="yes"
  />
    <xsl:template match="/test">
      <xsl:variable name="test_id" select="@id" />
      <xsl:document 
        href="test_{$test_id}.html"
        method="xml"
        doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"
        doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN"
        indent="yes"
      >

<html>
  <head>
    <title>
      ::<xsl:value-of select="@id" /> @ <xsl:value-of select="host" />::
    </title>
    <link rel="stylesheet" type="text/css" href="test_report.css" />
  </head>
<body>

<table>
<!-- the unit test row -->
  <tr class="heading">
    <td>
      <!-- test id -->
      <xsl:value-of select="@id" />
      <br />
      <xsl:value-of select="host" />
    </td>
  </tr>
  <tr>
    <td>
    <div class="syn">
      <table class="syn">
        <!-- test global attributes name/val pairs -->
        <xsl:for-each select="./*">
          <xsl:if test="not(name()='test_suite') and not(name()='host')">
          <tr>
            <td><xsl:value-of select="name()" />:</td>
            <td><xsl:value-of select="." /></td>
          </tr>
         </xsl:if>
        </xsl:for-each>
      </table>
    </div>
    </td>
  </tr>
<!-- /the unit test row -->
</table>

<table>
<!-- test suites' rows -->
  <xsl:for-each select="test_suite">
  <xsl:variable name="style_ts" select="status" />
  <tr class="framed">
    <td>
      <table class="{$style_ts}">
        <tr>
          <td class="ident"><xsl:value-of select="@id"/></td>
          <td>
            <table>
<!-- test suite global attributes name/val pairs -->
            <xsl:for-each select="./*">
              <xsl:if test="not(name()='test_case')">
              <tr>
                <td><xsl:value-of select="name()" />:</td>
                <td><xsl:value-of select="." /></td>
              </tr>
             </xsl:if>
            </xsl:for-each>
<!-- /test suite global attributes name/val pairs -->
            </table>
          </td>
        </tr>
      </table>
    </td>
<!-- test cases' data -->
    <td>
      <table>
<!-- test cases' rows -->
      <xsl:for-each select="test_case">
      <xsl:variable name="style_tc" select="status" />
      <!-- test case id -->
      <tr class="{$style_tc}">
        <td class="ident"><xsl:value-of select="@id"/></td>
        <td>
          <table>
<!-- test case global attributes name/val pairs -->
          <xsl:for-each select="./*">
          <tr>
            <td><xsl:value-of select="name()" />:</td>
            <td><xsl:value-of select="." /></td>
          </tr>
          </xsl:for-each>
<!-- /test case global attributes name/val pairs -->
          </table>
        </td>
      </tr>
      </xsl:for-each>
<!-- /test cases' rows -->
      </table>
    </td>
<!-- /test cases' data -->
  </tr>
  </xsl:for-each>
<!-- /test suites' rows -->

</table>
</body>
</html>

    </xsl:document>
  </xsl:template>
</xsl:stylesheet>

<!--
 vim: set filetype=xml expandtab tabstop=2 shiftwidth=2 autoindent smartindent:
 -->
