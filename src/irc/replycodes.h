/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef REPLYCODES_H
#define REPLYCODES_H

#define RPL_WELCOME            001
#define RPL_YOURHOST           002
#define RPL_CREATED            003
#define RPL_MYINFO             004
#define RPL_BOUNCE             005                // old RFC1459 definition
#define RPL_ISUPPORT           005                // new DALnet definition

#define RPL_TRACELINK          200
#define RPL_TRACECONNECTING    201
#define RPL_TRACEHANDSHAKE     202
#define RPL_TRACEUNKNOWN       203
#define RPL_TRACEOPERATOR      204
#define RPL_TRACEUSER          205
#define RPL_TRACESERVER        206
#define RPL_TRACENEWTYPE       208
#define RPL_STATSLINKINFO      211
#define RPL_STATSCOMMANDS      212
#define RPL_STATSCLINE         213
#define RPL_STATSNLINE         214
#define RPL_STATSILINE         215
#define RPL_STATSKLINE         216
#define RPL_STATSYLINE         218
#define RPL_ENDOFSTATS         219
#define RPL_UMODEIS            221
#define RPL_STATSLLINE         241
#define RPL_STATSUPTIME        242
#define RPL_STATSOLINE         243
#define RPL_STATSHLINE         244
#define RPL_HIGHCONNECTCOUNT   250
#define RPL_LUSERCLIENT        251
#define RPL_LUSEROP            252
#define RPL_LUSERUNKNOWN       253
#define RPL_LUSERCHANNELS      254
#define RPL_LUSERME            255
#define RPL_ADMINME            256
#define RPL_ADMINLOC1          257
#define RPL_ADMINLOC2          258
#define RPL_ADMINEMAIL         259
#define RPL_TRACELOG           261
#define RPL_LOCALUSERS         265
#define RPL_GLOBALUSERS        266
#define RPL_CAPAB              290

#define RPL_NONE               300
#define RPL_AWAY               301
#define RPL_USERHOST           302
#define RPL_ISON               303
#define RPL_UNAWAY             305
#define RPL_NOWAWAY            306
#define RPL_WHOISIDENTIFY      307                // DALNet NickServ
#define RPL_WHOISHELPER        310
#define RPL_WHOISUSER          311
#define RPL_WHOISSERVER        312
#define RPL_WHOISOPERATOR      313
#define RPL_WHOWASUSER         314
#define RPL_ENDOFWHO           315
#define RPL_WHOISIDLE          317
#define RPL_ENDOFWHOIS         318
#define RPL_WHOISCHANNELS      319
#define RPL_IDENTIFIED         320                // when you do /whois you can get  [320] JohnFlux is an identified user
#define RPL_LISTSTART          321
#define RPL_LIST               322
#define RPL_LISTEND            323
#define RPL_CHANNELMODEIS      324
#define RPL_CHANNELURLIS       328
#define RPL_CHANNELCREATED     329
#define RPL_WHOISACCOUNT       330
#define RPL_NOTOPIC            331
#define RPL_TOPIC              332
#define RPL_TOPICSETBY         333                // Extended ircd
#define RPL_WHOISACTUALLY      338
#define RPL_INVITING           341
#define RPL_SUMMONING          342
#define RPL_VERSION            351
#define RPL_WHOREPLY           352
#define RPL_NAMREPLY           353
#define RPL_WHOSPCRPL          354
#define RPL_LINKS              364
#define RPL_ENDOFLINKS         365
#define RPL_ENDOFNAMES         366
#define RPL_BANLIST            367
#define RPL_ENDOFBANLIST       368
#define RPL_ENDOFWHOWAS        369
#define RPL_INFO               371
#define RPL_MOTD               372
#define RPL_ENDOFINFO          374
#define RPL_MOTDSTART          375
#define RPL_ENDOFMOTD          376
#define RPL_YOUREOPER          381
#define RPL_REHASHING          382
#define RPL_TIME               391
#define RPL_USERSSTART         392
#define RPL_USERS              393
#define RPL_ENDOFUSERS         394
#define RPL_NOUSERS            395
#define RPL_HOSTHIDDEN         396

#define ERR_NOSUCHNICK         401
#define ERR_NOSUCHSERVER       402
#define ERR_NOSUCHCHANNEL      403
#define ERR_CANNOTSENDTOCHAN   404
#define ERR_TOOMANYCHANNELS    405
#define ERR_WASNOSUCHNICK      406
#define ERR_TOOMANYTARGETS     407
#define ERR_NOORIGIN           409
#define ERR_NORECIPIENT        411
#define ERR_NOTEXTTOSEND       412
#define ERR_NOTOPLEVEL         413
#define ERR_WILDTOPLEVEL       414
#define ERR_UNKNOWNCOMMAND     421
#define ERR_NOMOTD             422
#define ERR_NOADMININFO        423
#define ERR_FILEERROR          424
#define ERR_NONICKNAMEGIVEN    431
#define ERR_ERRONEUSNICKNAME   432
#define ERR_NICKNAMEINUSE      433
#define ERR_NICKCOLLISION      436
#define ERR_UNAVAILRESOURCE    437
#define ERR_USERNOTINCHANNEL   441
#define ERR_NOTONCHANNEL       442
#define ERR_USERONCHANNEL      443
#define ERR_NOLOGIN            444
#define ERR_SUMMONDISABLED     445
#define ERR_USERSDISABLED      446
#define ERR_NOTREGISTERED      451
#define ERR_NEEDMOREPARAMS     461
#define ERR_ALREADYREGISTRED   462
#define ERR_NOPERMFORHOST      463
#define ERR_PASSWDMISMATCH     464
#define ERR_YOUREBANNEDCREEP   465
#define ERR_KEYSET             467
#define ERR_CHANNELISFULL      471
#define ERR_UNKNOWNMODE        472
#define ERR_INVITEONLYCHAN     473
#define ERR_BANNEDFROMCHAN     474
#define ERR_BADCHANNELKEY      475
#define ERR_NOCHANMODES        477
#define ERR_NOPRIVILEGES       481
#define ERR_CHANOPRIVSNEEDED   482
#define ERR_CANTKILLSERVER     483
#define ERR_NOOPERHOST         491
#define ERR_UMODEUNKNOWNFLAG   501
#define ERR_USERSDONTMATCH     502

#define RPL_WHOISSECURE        671 // used by UnrealIRCd to denote SSL in WHOIS response

#define RPL_LOGGEDIN           900
#define RPL_LOGGEDOUT          901
#define ERR_NICKLOCKED         902
#define RPL_SASLSUCCESS        903
#define ERR_SASLFAIL           904
#define ERR_SASLTOOLONG        905
#define ERR_SASLABORTED        906
#define ERR_SASLALREADY        907
#define RPL_SASLMECHS          908

#endif // REPLYCODES_H

