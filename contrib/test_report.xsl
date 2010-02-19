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
    doctype-public="-//W3C/DTD XHTML 1.0 Strict//EN"
    indent="yes"
  />
    <xsl:template match="/test">
      <xsl:variable name="test_id" select="@id" />
      <xsl:document 
        href="test_{$test_id}.html"
        method="xml"
        doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"
        doctype-public="-//W3C/DTD XHTML 1.0 Strict//EN"
        indent="yes"
      >

<html>
<head>
<link rel="stylesheet" type="text/css" href="test_report.css" />
</head>
<body>

<div id="test_table">
<table>
<thead>
  <th><xsl:value-of select="@id" /> Unit Test Report</th>
</thead>
<tbody>
  <xsl:for-each select="test_suite">
  <xsl:variable name="style_ts" select="status" />
  <table class="{$style_ts}">
  <th><xsl:value-of select="@id"/></th>
    <td>
      <table class="{$style_ts}">
      <xsl:for-each select="test_case">
      <xsl:variable name="style_tc" select="status" />
        <tr>
          <th><xsl:value-of select="@id"/></th>
          <td>
            <table class="{$style_tc}">
              <xsl:for-each select="*">
              <tr>
                <td><xsl:value-of select="name()" />:</td>
                <td><xsl:value-of select="." /></td>
              </tr>
              </xsl:for-each>
            </table>
          </td>
        </tr>
      </xsl:for-each>
      </table>
    </td>
  </table>
  </xsl:for-each>
</tbody>
</table>
</div>

</body>
</html>

    </xsl:document>
  </xsl:template>
</xsl:stylesheet>
