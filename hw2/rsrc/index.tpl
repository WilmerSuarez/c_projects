<HTML>
<HEAD>
<TITLE>Index of Persons</TITLE>
</HEAD>
<BODY>
<H1>Index of Persons</H1>
<P>
!WHILE @
!IF @.TITLE
<A HREF="${@.&}">${@.NAME} (${@.TITLE})</A>
!ELSE
<A HREF="${@.&}">${@.NAME}</A>
!ENDIF
 (${@.BIRTH.DATE} - ${@.DEATH.DATE})<BR>
!NEXT
!END
</P>
</BODY>
</HTML>
