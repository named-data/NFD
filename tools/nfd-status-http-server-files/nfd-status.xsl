<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:nfd="ndn:/localhost/nfd/status/1">
<xsl:output method="html" encoding="UTF-8" doctype-system="about:legacy-compat"/>

<xsl:template match="/">
  <html>
  <head>
    <title>NFD Status</title>
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
  <header>
    <h1>NFD Status</h1>
  </header>
  <article>
    <div id="content">
      <xsl:apply-templates/>
    </div>
  </article>
  <footer>
    <xsl:variable name="version">
      <xsl:apply-templates select="nfd:nfdStatus/nfd:generalStatus/nfd:version"/>
    </xsl:variable>
    <span class="grey">Powered by </span><a target="_blank" href="https://named-data.net/doc/NFD/"><span class="green">NFD version <xsl:value-of select="$version"/></span></a><span class="grey">.</span>
  </footer>
  </body>
  </html>
</xsl:template>

<xsl:template name="formatDate">
  <xsl:param name="date" />
  <xsl:value-of select="substring($date, 0, 11)"/>&#160;<xsl:value-of select="substring($date, 12, 8)"/>
</xsl:template>

<xsl:template name="formatDuration">
  <xsl:param name="duration" />
  <xsl:variable name="milliseconds">
    <xsl:choose>
      <xsl:when test="contains($duration, '.')">
        <xsl:value-of select="substring($duration, string-length($duration)-4, 3)" />
      </xsl:when>
      <xsl:otherwise>
        0
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="seconds">
    <xsl:choose>
      <xsl:when test="contains($duration, '.')">
        <xsl:value-of select="substring($duration, 3, string-length($duration)-7)" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring($duration, 3, string-length($duration)-3)" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="days"><xsl:value-of select="floor($seconds div 86400)" /></xsl:variable>
  <xsl:variable name="hours"><xsl:value-of select="floor($seconds div 3600)" /></xsl:variable>
  <xsl:variable name="minutes"><xsl:value-of select="floor($seconds div 60)" /></xsl:variable>
  <xsl:choose>
    <xsl:when test="$days = 1">
      <xsl:value-of select="$days"/> day
    </xsl:when>
    <xsl:when test="$days > 1">
      <xsl:value-of select="$days"/> days
    </xsl:when>
    <xsl:when test="$hours = 1">
      <xsl:value-of select="$hours"/> hour
    </xsl:when>
    <xsl:when test="$hours > 1">
      <xsl:value-of select="$hours"/> hours
    </xsl:when>
    <xsl:when test="$minutes = 1">
      <xsl:value-of select="$minutes"/> minute
    </xsl:when>
    <xsl:when test="$minutes > 1">
      <xsl:value-of select="$minutes"/> minutes
    </xsl:when>
    <xsl:when test="$seconds = 1">
      <xsl:value-of select="$seconds"/> second
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$seconds"/> seconds
    </xsl:otherwise>
    <xsl:when test="$milliseconds > 1">
      <xsl:value-of select="$milliseconds"/> milliseconds
    </xsl:when>
    <xsl:when test="$milliseconds = 1">
      <xsl:value-of select="$milliseconds"/> millisecond
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="nfd:generalStatus">
  <h2>General Status</h2>
  <table class="item-list">
    <thead>
      <tr>
        <th>Version</th>
        <th>Start time</th>
        <th>Current time</th>
        <th>Uptime</th>
        <th>NameTree Entries</th>
        <th>FIB entries</th>
        <th>PIT entries</th>
        <th>Measurements entries</th>
        <th>CS entries</th>
        <th>In Interests</th>
        <th>Out Interests</th>
        <th>In Data</th>
        <th>Out Data</th>
        <th>In Nacks</th>
        <th>Out Nacks</th>
        <th>Satisfied Interests</th>
        <th>Unsatisfied Interests</th>
      </tr>
    </thead>
    <tbody>
      <tr class="center">
        <td><xsl:value-of select="nfd:version"/></td>
        <td><xsl:call-template name="formatDate"><xsl:with-param name="date" select="nfd:startTime" /></xsl:call-template></td>
        <td><xsl:call-template name="formatDate"><xsl:with-param name="date" select="nfd:currentTime" /></xsl:call-template></td>
        <td><xsl:call-template name="formatDuration"><xsl:with-param name="duration" select="nfd:uptime" /></xsl:call-template></td>
        <td><xsl:value-of select="nfd:nNameTreeEntries"/></td>
        <td><xsl:value-of select="nfd:nFibEntries"/></td>
        <td><xsl:value-of select="nfd:nPitEntries"/></td>
        <td><xsl:value-of select="nfd:nMeasurementsEntries"/></td>
        <td><xsl:value-of select="nfd:nCsEntries"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nInterests"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nInterests"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nData"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nData"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nNacks"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nNacks"/></td>
        <td><xsl:value-of select="nfd:nSatisfiedInterests"/></td>
        <td><xsl:value-of select="nfd:nUnsatisfiedInterests"/></td>
      </tr>
    </tbody>
  </table>
</xsl:template>

<xsl:template match="nfd:channels">
  <h2>Channels</h2>
  <table class="item-list alt-row-colors">
    <thead>
      <tr>
        <th>Channel URI</th>
      </tr>
    </thead>
    <tbody>
      <xsl:for-each select="nfd:channel">
      <tr>
        <td><xsl:value-of select="nfd:localUri"/></td>
      </tr>
      </xsl:for-each>
    </tbody>
  </table>
</xsl:template>

<xsl:template match="nfd:faces">
  <h2>Faces</h2>
  <table class="item-list alt-row-colors">
    <thead>
      <tr>
        <th>Face ID</th>
        <th>Remote URI</th>
        <th>Local URI</th>
        <th>Scope</th>
        <th>Persistency</th>
        <th>LinkType</th>
        <th>MTU</th>
        <th>Flags</th>
        <th>Expires in</th>
        <th>In Interests</th>
        <th>In Data</th>
        <th>In Nacks</th>
        <th>In Bytes</th>
        <th>Out Interests</th>
        <th>Out Data</th>
        <th>Out Nacks</th>
        <th>Out Bytes</th>
      </tr>
    </thead>
    <tbody>
      <xsl:for-each select="nfd:face">
      <tr>
        <td><xsl:value-of select="nfd:faceId"/></td>
        <td><xsl:value-of select="nfd:remoteUri"/></td>
        <td><xsl:value-of select="nfd:localUri"/></td>
        <td><xsl:value-of select="nfd:faceScope"/></td>
        <td><xsl:value-of select="nfd:facePersistency"/></td>
        <td><xsl:value-of select="nfd:linkType"/></td>
        <td>
          <xsl:choose>
            <xsl:when test="nfd:mtu">
              <xsl:value-of select="nfd:mtu"/>
            </xsl:when>
            <xsl:otherwise>
              n/a
            </xsl:otherwise>
          </xsl:choose>
        </td>
        <td>
          <xsl:if test="nfd:flags/nfd:localFieldsEnabled">local-fields </xsl:if>
          <xsl:if test="nfd:flags/nfd:lpReliabilityEnabled">reliability </xsl:if>
          <xsl:if test="nfd:flags/nfd:congestionMarkingEnabled">congestion-marking </xsl:if>
        </td>
        <td>
          <xsl:choose>
            <xsl:when test="nfd:expirationPeriod">
              <xsl:call-template name="formatDuration"><xsl:with-param name="duration" select="nfd:expirationPeriod" /></xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              Never
            </xsl:otherwise>
          </xsl:choose>
        </td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nInterests"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nData"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:incomingPackets/nfd:nNacks"/></td>
        <td><xsl:value-of select="nfd:byteCounters/nfd:incomingBytes"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nInterests"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nData"/></td>
        <td><xsl:value-of select="nfd:packetCounters/nfd:outgoingPackets/nfd:nNacks"/></td>
        <td><xsl:value-of select="nfd:byteCounters/nfd:outgoingBytes"/></td>
      </tr>
      </xsl:for-each>
    </tbody>
  </table>
</xsl:template>

<xsl:template match="nfd:fib">
  <h2>FIB</h2>
  <table class="item-list alt-row-colors">
    <thead>
      <tr>
        <th class="name-prefix">Prefix</th>
        <th>NextHops</th>
      </tr>
    </thead>
    <tbody>
      <xsl:for-each select="nfd:fibEntry">
      <tr>
        <td style="text-align:left;vertical-align:top;padding:0"><xsl:value-of select="nfd:prefix"/></td>
        <td>
          <table class="item-sublist">
            <tr>
              <th>FaceId</th>
              <xsl:for-each select="nfd:nextHops/nfd:nextHop">
                <td><xsl:value-of select="nfd:faceId"/></td>
              </xsl:for-each>
            </tr>
            <tr>
              <th>Cost</th>
              <xsl:for-each select="nfd:nextHops/nfd:nextHop">
                <td><xsl:value-of select="nfd:cost"/></td>
              </xsl:for-each>
            </tr>
          </table>
        </td>
      </tr>
      </xsl:for-each>
    </tbody>
  </table>
</xsl:template>

<xsl:template match="nfd:rib">
  <h2>RIB</h2>
  <table class="item-list alt-row-colors">
    <thead>
      <tr>
        <th class="name-prefix">Prefix</th>
        <th>Routes</th>
      </tr>
    </thead>
    <tbody>
      <xsl:for-each select="nfd:ribEntry">
      <tr>
        <td style="text-align:left;vertical-align:top;padding:0"><xsl:value-of select="nfd:prefix"/></td>
        <td>
          <table class="item-sublist">
            <tr>
              <th>FaceId</th>
              <xsl:for-each select="nfd:routes/nfd:route">
                <td><xsl:value-of select="nfd:faceId"/></td>
              </xsl:for-each>
            </tr>
            <tr>
              <th>Origin</th>
              <xsl:for-each select="nfd:routes/nfd:route">
                <td><xsl:value-of select="nfd:origin"/></td>
              </xsl:for-each>
            </tr>
            <tr>
              <th>Cost</th>
              <xsl:for-each select="nfd:routes/nfd:route">
                <td><xsl:value-of select="nfd:cost"/></td>
              </xsl:for-each>
            </tr>
            <tr>
              <th>ChildInherit</th>
              <xsl:for-each select="nfd:routes/nfd:route">
                <td>
                  <xsl:if test="nfd:flags/nfd:childInherit">Y</xsl:if>
                </td>
              </xsl:for-each>
            </tr>
            <tr>
              <th>RibCapture</th>
              <xsl:for-each select="nfd:routes/nfd:route">
                <td>
                  <xsl:if test="nfd:flags/nfd:ribCapture">Y</xsl:if>
                </td>
              </xsl:for-each>
            </tr>
            <tr>
              <th>Expires in</th>
              <xsl:for-each select="nfd:routes/nfd:route">
                <td>
                  <xsl:choose>
                    <xsl:when test="nfd:expirationPeriod">
                      <xsl:call-template name="formatDuration"><xsl:with-param name="duration" select="nfd:expirationPeriod" /></xsl:call-template>
                    </xsl:when>
                    <xsl:otherwise>
                      Never
                    </xsl:otherwise>
                  </xsl:choose>
                </td>
              </xsl:for-each>
            </tr>
          </table>
        </td>
      </tr>
      </xsl:for-each>
    </tbody>
  </table>
</xsl:template>

<xsl:template match="nfd:cs">
  <h2>Content Store</h2>
  <table class="item-list">
    <thead>
      <tr>
        <th>Enablement Flags</th>
        <th>Capacity</th>
        <th>Entries</th>
        <th>Hits</th>
        <th>Misses</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>
          <xsl:choose>
            <xsl:when test="nfd:admitEnabled">admit</xsl:when>
            <xsl:otherwise>no-admit</xsl:otherwise>
          </xsl:choose>
          ,
          <xsl:choose>
            <xsl:when test="nfd:serveEnabled">serve</xsl:when>
            <xsl:otherwise>no-serve</xsl:otherwise>
          </xsl:choose>
        </td>
        <td><xsl:value-of select="nfd:capacity"/></td>
        <td><xsl:value-of select="nfd:nEntries"/></td>
        <td><xsl:value-of select="nfd:nHits"/></td>
        <td><xsl:value-of select="nfd:nMisses"/></td>
      </tr>
    </tbody>
  </table>
</xsl:template>

<xsl:template match="nfd:strategyChoices">
  <h2>Strategy Choices</h2>
  <table class="item-list alt-row-colors">
    <thead>
      <tr>
        <th class="name-prefix">Namespace</th>
        <th>Strategy Name</th>
      </tr>
    </thead>
    <tbody>
      <xsl:for-each select="nfd:strategyChoice">
      <tr>
        <td><xsl:value-of select="nfd:namespace"/></td>
        <td><xsl:value-of select="nfd:strategy/nfd:name"/></td>
      </tr>
      </xsl:for-each>
    </tbody>
  </table>
</xsl:template>

</xsl:stylesheet>
