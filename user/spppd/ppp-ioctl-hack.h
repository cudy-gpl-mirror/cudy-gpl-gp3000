#ifdef CONFIG_PPPOE_PROXY
#define	PPPIOGETFROMPORT	_IOR('t', 95, int)	/* get ppp channel fromport */
typedef char peermacaddr[6];
#define	PPPIOCSPEERMAC	_IOW('t', 94, peermacaddr)	/* set pppoe proxy peermac */
#define	PPPIOCSSESSION	_IOW('t', 93, int)	/* set pppoe proxy session id */
#endif

/*linux-2.6.19*/
/*patch from linux 2.4*/
#define	PPPIOCGTIMEOUT	_IOR('t', 92, int)	/* get configuration flags */
#define	PPPIOCSTIMEOUT	_IOW('t', 91, int)	/* set configuration flags */

