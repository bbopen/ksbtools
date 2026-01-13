/* File:         opcapitest.c
 * RCS:          $Header: /usr/msrc/usr/local/etc/tpsh/RCS/opcapitest.c,v 2.0 1996/11/07 17:35:40 kb207252 Exp $
 * Description:  This file contains a sample program using the OpC API to send
 *               messages and monitor values.
 * Language:     K&R C
 * Package:      HP OpenView OperationsCenter
 * Status:       @(#)OpC A.01.10 (08/05/94)
 *
 * (c) Copyright 1993, Hewlett-Packard Company, all rights reserved.
 */
#include <stdio.h>
#include <opcapi.h>

/*
 * Function: report_error
 *
 * Description:
 *       Maps an OpC error code to an error text and prints this text to
 *       stderr.
 * 
 * Input Parameters:
 *       err_code           OpC error code.
 */
static void
report_error(err_code)
int err_code;
{
	static struct err_tbl_entry {
		int  e_code;
		char *err_text;
	} err_tbl[] = {
		{ OPC_ERR_OK,                  "No error." },
		{ OPC_ERR_APPL_REQUIRED,       "Application parameter required." },
		{ OPC_ERR_OBJ_REQUIRED,        "Object parameter required." },
		{ OPC_ERR_TEXT_REQUIRED,       "Message text parameter required." },
		{ OPC_ERR_INVAL_SEVERITY,      "Invalid severity value." },
		{ OPC_ERR_OBJNAME_REQUIRED,    "Object name required." },
		{ OPC_ERR_MISC_NOT_ALLOWED,    "Message group 'Misc' not allowed." },
		{ OPC_ERR_INVAL_NODE,          "Unknown node specified." },
		{ OPC_ERR_NO_MEMORY,           "Insufficient memory." },
		{ OPC_ERR_NO_AGENT,            "The responsible OpC agent is not running." },
		{ OPC_ERR_CANT_INIT,           "OpC error: Can't initialize." },
		{ OPC_ERR_NO_QUEUE_NAME,       "OpC error: Can't get name of value queue." },
		{ OPC_ERR_CANT_OPEN_QUEUE,     "OpC error: Can't open value queue." },
		{ OPC_ERR_CANT_WRITE_QUEUE,    "OpC error: Can't write value queue." },
		{ OPC_ERR_NO_PIPE_NAME,        "OpC error: Can't get name of value pipe." },
		{ OPC_ERR_CANT_OPEN_PIPE,      "OpC error: Can't open value pipe." },
		{ OPC_ERR_CANT_GET_LOCAL_ADDR, "OpC error: Can't get local IP address." }
	};
	int i;

	/* Look for the error code and print the corresponding error text. */

	for (i = 0; i < (sizeof(err_tbl)/sizeof(struct err_tbl_entry)); i++) {
		if (err_code == err_tbl[i].e_code) {
			fprintf(stderr, "%s\n", err_tbl[i].err_text);
			return;
		}
	}

	fprintf(stderr, "Unknown error: %d\n", err_code);
}

/* Function: main
 *
 * Description:
 *       Main routine of the apitest program. A menu is printed and the user
 *       can enter whether to send an OpC message or a monitor value. The
 *       user is prompted to enter needed parameters.
 * 
 * Input Parameters:
 *       argc, argv          Default main() parameters. Unused.
 */
int
main(argc, argv)
int  argc;
char **argv;
{
	char  c_buf[10], msggrp[256];
	char  appl[256], object[256];
	char  text[256], severity[256];
	char  node[256], floatval[256];
	char  *msggrppt, *applpt, *objectpt, *textpt, *severitypt;
	char  *nodept, *floatvalpt;
	int   sev, rc, choice;
	double val;

	for (;;) {
		printf("\nExit ................. 0\n");
		printf("Send message ......... 1\n");
		printf("Send monitor value ... 2\n> ");
		fflush(stdout);

		if (fgets(c_buf, sizeof(c_buf), stdin) == NULL) {
			exit(0);
		}

		/* Try to translate the input into numeric value. */

		if (sscanf(c_buf, "%d", &choice) < 1) {
			fprintf(stderr, "Invalid choice. Ignored\n");
			continue;
		}

		switch (choice) {
		case 0: /* Exit. */
			exit(0);

		case 1: /* Send an OpC message. */
			printf("Please enter message attributes.\n");

			printf("Application: "); applpt = gets(appl);
			clearerr(stdin);
			printf("Object: ");      objectpt = gets(object);
			clearerr(stdin);
			printf("Text: ");        textpt = gets(text);
			clearerr(stdin);
			printf("Msg group: ");   msggrppt = gets(msggrp);
			clearerr(stdin);
			printf("Severity: ");    gets(severity);
			clearerr(stdin);
			printf("Node: ");        nodept = gets(node);

			/* Translate severity into numeric value. */

			if (0 == strcasecmp(severity, "normal"))
				sev = OPC_SEV_NORMAL;
			else if (0 == strcasecmp(severity, "warning"))
				sev = OPC_SEV_WARNING;
			else if (0 == strcasecmp(severity, "critical"))
				sev = OPC_SEV_CRITICAL;
			else
				sev = OPC_SEV_UNKNOWN;

			/* Send the message. */

			rc = opcmsg(sev, applpt, objectpt, textpt, msggrppt, nodept);

			/* Print an error if necessary. */
			if (rc != OPC_ERR_OK) {
				  fprintf(stderr, "Sending OpC message failed\n");
				  report_error(rc);
			}
			break;

		case 2: /* Send an OpC monitor value. */
			printf("Object: "); objectpt = gets(object);
			clearerr(stdin);
			printf("Value: ");  gets(floatval);
			clearerr(stdin);
			sscanf(floatval, "%lf", &val);

			rc = opcmon(objectpt, val);

			if (rc != OPC_ERR_OK) {
				fprintf(stderr, "Sending OpC monitor value failed\n");
				report_error(rc);
			}
			break;

		default: /* else */
			fprintf(stderr, "Invalid choice. Ignored\n");
			continue;
		}
	}
	exit(0);
}
