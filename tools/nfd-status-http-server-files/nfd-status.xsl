<xsl:stylesheet version="1.0"
xmlns="http://www.w3.org/1999/xhtml"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:nfd="ndn:/localhost/nfd/status/1">

<xsl:template match="/">
  <html>
  <head>
  <title>NFD Status</title>
  </head>
  <body>
  <xsl:apply-templates/>
  </body>
  </html>
</xsl:template>

<xsl:template match="nfd:generalStatus">
  <h2>General NFD status</h2>
  <table>
    <tr>
      <td>Version</td>
      <td><xsl:value-of select="nfd:version"/></td>
    </tr>
    <tr>
      <td>startTime</td>
      <td><xsl:value-of select="nfd:startTime"/></td>
    </tr>
    <tr>
      <td>currentTime</td>
      <td><xsl:value-of select="nfd:currentTime"/></td>
    </tr>
    <tr>
      <td>upTime</td>
      <td><xsl:value-of select="nfd:uptime"/></td>
    </tr>
    <tr>
      <td>nNameTreeEntries</td>
      <td><xsl:value-of select="nfd:nNameTreeEntries"/></td>
    </tr>
    <tr>
      <td>nFibEntries</td>
      <td><xsl:value-of select="nfd:nFibEntries"/></td>
    </tr>
    <tr>
      <td>nPitEntries</td>
      <td><xsl:value-of select="nfd:nPitEntries"/></td>
    </tr>
    <tr>
      <td>nMeasurementsEntries</td>
      <td><xsl:value-of select="nfd:nMeasurementsEntries"/></td>
    </tr>
    <tr>
      <td>nCsEntries</td>
      <td><xsl:value-of select="nfd:nCsEntries"/></td>
    </tr>
    <tr>
      <td>nInInterests</td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nInterests"/></td>
    </tr>
    <tr>
      <td>nOutInterests</td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nInterests"/></td>
    </tr>
    <tr>
      <td>nInDatas</td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nDatas"/></td>
    </tr>
    <tr>
      <td>nOutDatas</td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nDatas"/></td>
    </tr>
  </table>
</xsl:template>

<xsl:template match="nfd:channels">
  <h2>Channels</h2>
  <table>
    <xsl:for-each select="nfd:channel">
    <tr>
      <td><xsl:value-of select="nfd:localUri"/></td>
    </tr>
    </xsl:for-each>
  </table>
</xsl:template>

<xsl:template match="nfd:faces">
  <h2>Faces</h2>
  <table>
    <tr style="background-color: #9acd32;">
      <th>faceID</th>
      <th>remoteUri</th>
      <th>localUri</th>
      <th>nInInterests</th>
      <th>nInDatas</th>
      <th>nOutInterests</th>
      <th>nOutDatas</th>
    </tr>
    <xsl:for-each select="nfd:face">
    <tr>
      <td><xsl:value-of select="nfd:faceId"/></td>
      <td><xsl:value-of select="nfd:remoteUri"/></td>
      <td><xsl:value-of select="nfd:localUri"/></td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nInterests"/></td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nDatas"/></td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nInterests"/></td>
      <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nDatas"/></td>
    </tr>
    </xsl:for-each>
  </table>
</xsl:template>

<xsl:template match="nfd:fib">
  <h2>FIB</h2>
  <table>
    <tr style="background-color: #9acd32;">
      <th>prefix</th>
      <th>nextHops</th>
    </tr>
    <xsl:for-each select="nfd:fibEntry">
    <tr>
      <td style="text-align:left;vertical-align:top;padding:0"><xsl:value-of select="nfd:prefix"/></td>
      <td>
      <xsl:for-each select="nfd:nextHops/nfd:nextHop">
        faceid=<xsl:value-of select="nfd:faceId"/> (cost=<xsl:value-of select="nfd:cost"/>);
      </xsl:for-each>
      </td>
    </tr>
    </xsl:for-each>
  </table>
</xsl:template>

<xsl:template match="nfd:strategyChoices">
  <h2>Strategy Choices</h2>
  <table>
    <tr style="background-color: #9acd32;">
      <th>Namespace</th>
      <th>Strategy Name</th>
    </tr>
    <xsl:for-each select="nfd:strategyChoice">
    <tr>
      <td><xsl:value-of select="nfd:namespace"/></td>
      <td><xsl:value-of select="nfd:strategy/nfd:name"/></td>
    </tr>
    </xsl:for-each>
  </table>
</xsl:template>

</xsl:stylesheet>
