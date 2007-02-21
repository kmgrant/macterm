/*
**************************************************************************
	ApplicationMisc.r
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains all resource data not found in other specialized
	".r" files.
**************************************************************************
*/

#ifndef __APPLICATIONMISC_R__
#define __APPLICATIONMISC_R__

#include "Carbon.r"
#include "CoreServices.r"

#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "PreferenceResources.h"

#include "SpacingConstants.r"
#include "TelnetButtonNames.h"
#include "TelnetMenuItemNames.h"
#include "TelnetMiscellaneousText.h"



/*
temporary - any older dialogs that use color boxes need a Rez-dependent control
*/
resource 'CNTL' (kIDRezDITLColorBox, "Rez-DITL Color Box", purgeable)
{
	{ 0, 0, 18, 48 },
	kControlHandlesTracking, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};

/* Translation Table Resources - 'taBL' resources are "user" tables and 'TRSL' resources 
   are "factory" provided tables (which are preloaded) */

type 'taBL'
{
	hex string[256];	// in table
	hex string[256];	// out table
};

//resource 'TMPL' (kTemplateBaseID + 11, "taBL", purgeable)
//{
//	{
//		"In translation table", 'H100',		// 256 byte hex string
//		"Out translation table", 'H100'		// 256 byte hex string
//	}
//};

resource 'taBL' (128, "US ASCII")
{
	// to Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"4141 4345 4E4F 5561 6161 6161 6163 6565"            /* AACENOUaaaaaacee */
	$"6565 6969 6969 6E6F 6F6F 6F6F 7575 7575"            /* eeiiiinooooouuuu */
	$"2B6F 634C 536F 5053 5243 2260 2223 414F"            /* +ocLSoPSRC"`"#AO */
	$"3823 3C3E 5975 6453 5070 5361 6F4F 616F"            /* 8#<>YudSPpSaoOao */
	$"3F69 2D56 663D 643C 3E2E 2041 414F 4F6F"            /* ?i-Vf=d<>. AAOOo */
	$"2D2D 2222 6060 2F6F 793D 2020 2020 2020"            /* --""``/oy=       */
	$"2020 2020 2020 2020 2020 2020 2020 2020"            /*                  */
	$"2020 2020 2020 2020 2020 2020 2020 2020",           /*                  */
	// from Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"4141 4345 4E4F 5561 6161 6161 6163 6565"            /* AACENOUaaaaaacee */
	$"6565 6969 6969 6E6F 6F6F 6F6F 7575 7575"            /* eeiiiinooooouuuu */
	$"2B6F 634C 536F 5053 5243 5427 2223 414F"            /* +ocLSoPSRCT'"#AO */
	$"3823 3C3E 5975 6453 5070 5361 6F4F 616F"            /* 8#<>YudSPpSaoOao */
	$"3F21 2D56 663D 643C 3E2E 2041 414F 4F6F"            /* ?!-Vf=d<>. AAOOo */
	$"2D2D 2222 2727 2F6F 7959 2F6F 3C3E 6666"            /* --""''/oyY/o<>ff */
	$"2B2E 2722 2541 4541 4545 4949 4949 4F4F"            /* +.'"%AEAEEIIIIOO */
	$"204F 5555 5569 5E7E 2D5E 2E6F 2C22 2C5E"            /*  OUUUi^~-^.o,",^ */
};

resource 'taBL' (129, "ISO 8859-1")
{
	// to Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"8081 8283 8485 8687 8889 8A8B 8C8D 8E8F"            /* €‚ƒ„…†‡ˆ‰Š‹Œ */
	$"9091 9293 9495 9697 9899 9A9B 9C9D 209F"            /* ‘’“”•–—˜™š›œ Ÿ */
	$"CAC1 A2A3 DBB4 CFA4 ACA9 BBC7 C2D0 A8F8"            /* ÊÁ¢£Û´Ï¤¬©»ÇÂĞ¨ø */
	$"A1B1 D3D2 ABB5 A6E1 FCD5 BCC8 B9B8 B2C0"            /* ¡±ÓÒ«µ¦áüÕ¼È¹¸²À */
	$"CBE7 E5CC 8081 AE82 E983 E6E8 EDEA EBEC"            /* ËçåÌ€®‚éƒæèíêëì */
	$"DC84 F1EE EFCD 85D7 AFF4 F2F3 86A0 DEA7"            /* Ü„ñîïÍ…×¯ôòó† Ş§ */
	$"8887 898B 8A8C BE8D 8F8E 9091 9392 9495"            /* ˆ‡‰‹ŠŒ¾‘“’”• */
	$"DD96 9897 999B 9AD6 BF9D 9C9E 9FE0 DFD8",           /* İ–˜—™›šÖ¿œŸàßØ */
	// from Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"C4C5 C7C9 D1D6 DCE1 E0E2 E4E3 E5E7 E9E8"            /* ÄÅÇÉÑÖÜáàâäãåçéè */
	$"EAEB EDEC EEEF F1F3 F2F4 F6F5 FAF9 FBFC"            /* êëíìîïñóòôöõúùûü */
	$"DDB0 A2A3 A720 B6DF AEA9 20B4 A820 C6D8"            /* İ°¢£§ ¶ß®© ´¨ ÆØ */
	$"20B1 BE20 A5B5 2020 BDBC 20AA BA20 E6F8"            /*  ±¾ ¥µ  ½¼ ªº æø */
	$"BFA1 AC20 2020 20AB BB20 A0C0 C3D5 20A6"            /* ¿¡¬    «»  ÀÃÕ ¦ */
	$"AD20 B3B2 20B9 F7D7 FF20 20A4 D0F0 DEFE"            /* ­ ³² ¹÷×ÿ  ¤ĞğŞş */
	$"FDB7 2020 20C2 CAC1 CBC8 CDCE CFCC D3D4"            /* ı·   ÂÊÁËÈÍÎÏÌÓÔ */
	$"20D2 DADB D920 2020 AF20 2020 B820 2020"            /*  ÒÚÛÙ   ¯   ¸    */
};

resource 'taBL' (130, "ISO 8859-2")
{
	// to Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"A5AA E266 E3C9 A0A0 C35F E1DC E5E8 EB8F"            /* ¥ªâfãÉ  Ã_áÜåèë */
	$"C6D4 D5D2 D3A5 D0D1 5FAA E4DD E6E9 EC90"            /* ÆÔÕÒÓ¥ĞÑ_ªäİæéì */
	$"CA41 FF4C DB4C 53A4 ACE1 E5E8 8F2D EBFB"            /* ÊAÿLÛLS¤¬áåè-ëû */
	$"A188 2CB8 27BC E6FF 2CE4 E6E9 90AC ECFD"            /* ¡ˆ,¸'¼æÿ,äæé¬ìı */
	$"D9E7 81E7 80BD 8C8C 8983 A296 9DEA B191"            /* Ùçç€½ŒŒ‰ƒ¢–ê±‘ */
	$"91C1 C5EE EFCC 85D7 DBF1 F2F4 86F8 E8A7"            /* ‘ÁÅîïÌ…×Ûñòô†øè§ */
	$"DA87 8287 8ABE 8D8D 8B8E AB98 9E92 B493"            /* Ú‡‚‡Š¾‹«˜’´“ */
	$"93C4 CB97 99CE 9AD6 DEF3 9CF5 9FF9 E9AC",           /* “ÄË—™ÎšÖŞóœõŸùé¬ */
	// from Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"C4C2 E2C9 A1D6 DCE1 B1C8 E4E8 C6E6 E9AC"            /* ÄÂâÉ¡ÖÜá±ÈäèÆæé¬ */
	$"BCCF EDEF CCEC CBF3 EBF4 F6F5 FACC ECFC"            /* ¼ÏíïÌìËóëôöõúÌìü */
	$"2BB0 CA4C A72A A7DF 5263 99EA A85F 67CD"            /* +°ÊL§*§ßRc™ê¨_gÍ */
	$"EDCE 3C3E EE4B F05F B3A5 B5A5 B5C5 E5D2"            /* íÎ<>îKğ_³¥µ¥µÅåÒ */
	$"F2D1 2D5F F1D2 5F22 225F A0F2 D5D5 F5D4"            /* òÑ-_ñÒ_""_ òÕÕõÔ */
	$"2D2D 2222 2727 F7D7 F4C0 E0D8 2727 F8D8"            /* --""''÷×ôÀàØ''øØ */
	$"F8A9 2722 B9A6 B6C1 ABBB CDAE BEDB D3D4"            /* ø©'"¹¦¶Á«»Í®¾ÛÓÔ */
	$"FBD9 DAF9 DBFB D9F9 DDFD 6BAF A3BF 47B7"            /* ûÙÚùÛûÙùİık¯£¿G· */
};

resource 'taBL' (140, "PC 850")
{
	// to Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"829F 8E89 8A88 8C8D 9091 8F95 9493 8081"            /* ‚Ÿ‰ŠˆŒ‘•”“€ */
	$"83BE AE99 9A98 9E9D D885 86BF A3AF D7C4"            /* ƒ¾®™š˜Ø…†¿£¯×Ä */
	$"8792 979C 9684 BBBC C0A8 C2B8 B9C1 C7C8"            /* ‡’—œ–„»¼À¨Â¸¹ÁÇÈ */
	$"FDFE FFC6 F7E7 E5CB A9BD A5B0 B7A2 B4E3"            /* ışÿÆ÷çåË©½¥°·¢´ã */
	$"E4FA F9F6 CEFB 8BCC B3AD C5C3 BAAA C9DB"            /* äúùöÎû‹Ì³­ÅÃºªÉÛ */
	$"DDDC E6E8 E9F5 EAEB ECF0 E2D9 D4CF EDD1"            /* İÜæèéõêëìğâÙÔÏíÑ */
	$"EEA7 EFF1 9BCD B5DF DEF2 F3F4 E0A0 F8AB"            /* î§ïñ›ÍµßŞòóôà ø« */
	$"D0B1 B6B2 A6A4 D6FC A1AC E1D5 D2D3 DACA",           /* Ğ±¶²¦¤Öü¡¬áÕÒÓÚÊ */
	// from Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"8E8F 8090 A599 9AA0 8583 84C6 8687 828A"            /* €¥™š …ƒ„Æ†‡‚Š */
	$"8889 A18D 8C8B A4A2 9593 94E4 A397 9681"            /* ˆ‰¡Œ‹¤¢•“”ä£—– */
	$"EDF8 BD9C F5BA F4E1 A9B8 CDEF FEC9 929D"            /* íø½œõºôá©¸ÍïşÉ’ */
	$"BBF1 F3C8 BEE6 F2BC ABAC CCA6 A7B9 919B"            /* »ñóÈ¾æò¼«¬Ì¦§¹‘› */
	$"A8AA ACCB 9FCA B3AE AFCE FFB7 C7E5 C4DD"            /* ¨ª¬ËŸÊ³®¯Îÿ·ÇåÄİ */
	$"F0DF FCFD DCFB F69E 98DB FECF D1D0 E8E7"            /* ğßüıÜûö˜ÛşÏÑĞèç */
	$"ECFA DABF C0B6 D2B5 B3D4 D6D7 D8DE E0E2"            /* ìúÚ¿À¶Òµ³ÔÖ×ØŞàâ */
	$"D9E3 E9EA EBD5 C3B4 EEC2 C1C5 F7B0 B1B2"            /* ÙãéêëÕÃ´îÂÁÅ÷°±² */
};

resource 'taBL' (141, "NeXTStep")
{
	// to Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"CACB E7E5 CC80 8182 E983 E6E8 EDEA EBEC"            /* ÊËçåÌ€‚éƒæèíêëì */
	$"2084 F1EE EFCD 85F4 F2F3 8620 20B5 20D6"            /*  „ñîïÍ…ôòó†  µ Ö */
	$"A9C1 A2A3 DAB4 C4A4 DB20 D2C7 DCDD DEDF"            /* ©Á¢£Ú´Ä¤Û ÒÇÜİŞß */
	$"A8D0 A0E0 E120 A6A5 E2E3 D3C8 C9E4 C2C0"            /* ¨Ğ àá ¦¥âãÓÈÉäÂÀ */
	$"20D4 ABF6 F7F8 F9FA AC20 FBFC 20FD FEFF"            /*  Ô«ö÷øùú¬ ûü ışÿ */
	$"D1B1 2020 2088 8789 8B8A 8C8D 8F8E 9091"            /* Ñ±   ˆ‡‰‹ŠŒ‘ */
	$"93AE 92BB 9495 2096 20AF CEBC 9897 999B"            /* “®’»”• – ¯Î¼˜—™› */
	$"9ABE 9D9C 9EF5 9F20 20BF CFA7 20D8 2020",           /* š¾œõŸ  ¿Ï§ Ø   */
	// from Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"8586 8789 9196 9AD6 D5D7 D9D8 DADB DDDC"            /* …†‡‰‘–šÖÕ×ÙØÚÛİÜ */
	$"DEDF E2E0 E4E5 E7ED ECEE F0EF F3F2 F4F6"            /* Şßâàäåçíìîğïóòôö */
	$"B220 A2A3 A7B7 B6FB B0A0 20C2 C820 E1E9"            /* ² ¢£§·¶û°  ÂÈ áé */
	$"20D1 2020 A59D 2020 2020 20E3 EB20 F1F9"            /*  Ñ  ¥     ãë ñù */
	$"BFA1 BE20 A620 20AB BBBC 8081 8495 EAFA"            /* ¿¡¾ ¦  «»¼€„•êú */
	$"B1D0 AABA C120 9F20 FD20 A4A8 ACAD AEAF"            /* ±ĞªºÁ Ÿ ı ¤¨¬­®¯ */
	$"B3B4 B8B9 BD83 8A82 8B88 8D8E 8F8C 9394"            /* ³´¸¹½ƒŠ‚‹ˆŒ“” */
	$"2092 9899 97F5 C3C4 C5C6 C7CA CBCD CECF"            /*  ’˜™—õÃÄÅÆÇÊËÍÎÏ */
};

resource 'taBL' (142, "DEC Multinational")
{
	// to Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"8081 8283 8485 8687 8889 8A8B 8C8D 8E8F"            /* €‚ƒ„…†‡ˆ‰Š‹Œ */
	$"9091 9293 9495 9697 9899 9A9B 9C9D 9E9F"            /* ‘’“”•–—˜™š›œŸ */
	$"A0C1 A2A3 A4B4 A6A4 DBA9 BBC7 ACAD AEAF"            /*  Á¢£¤´¦¤Û©»Ç¬­®¯ */
	$"A1B1 B2B3 B4B5 A6E1 B8F5 BCC8 BCBD BEC0"            /* ¡±²³´µ¦á¸õ¼È¼½¾À */
	$"CBE7 E5CC 8081 AE82 E983 E6E8 EDEA EBEC"            /* ËçåÌ€®‚éƒæèíêëì */
	$"D084 F1EE EFCD 85CE AFF4 F2F3 86D9 DEA7"            /* Ğ„ñîïÍ…Î¯ôòó†ÙŞ§ */
	$"8887 898B 8A8C BE8D 8F8E 9091 9392 9495"            /* ˆ‡‰‹ŠŒ¾‘“’”• */
	$"F096 9897 999B 9ACF BF9D 9C9E 9FD8 FEFF",           /* ğ–˜—™›šÏ¿œŸØşÿ */
	// from Mac Roman
	$"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"            /* .........Æ...Â.. */
	$"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"            /* ................ */
	$"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"            /*  !"#$%&'()*+,-./ */
	$"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"            /* 0123456789:;<=>? */
	$"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"            /* @ABCDEFGHIJKLMNO */
	$"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"            /* PQRSTUVWXYZ[\]^_ */
	$"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"            /* `abcdefghijklmno */
	$"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"            /* pqrstuvwxyz{|}~. */
	$"C4C5 C7C9 D1D6 DCE1 E0E2 E4E3 E5E7 E9E8"            /* ÄÅÇÉÑÖÜáàâäãåçéè */
	$"EAEB EDEC EEEF F1F3 F2F4 F6F5 FAF9 FBFC"            /* êëíìîïñóòôöõúùûü */
	$"20B0 A2A3 A720 B6DF 20A9 2020 2020 C6D8"            /*  °¢£§ ¶ß ©    ÆØ */
	$"20B1 2020 A5B5 2020 2020 20AA BA20 E6F8"            /*  ±  ¥µ     ªº æø */
	$"BFA1 2020 2020 20AB BB20 20C0 C3D5 D7F7"            /* ¿¡     «»  ÀÃÕ×÷ */
	$"2020 2020 2020 2020 FDDD 20A8 2020 2020"            /*         ıİ ¨     */
	$"20B7 2020 20C2 CAC1 CBC8 CDCE CFCC D3D4"            /*  ·   ÂÊÁËÈÍÎÏÌÓÔ */
	$"20D2 DADB D9B9 2020 2020 2020 2020 2020"            /*  ÒÚÛÙ¹           */
};

#endif