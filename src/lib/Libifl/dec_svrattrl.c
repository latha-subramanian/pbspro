/*
 * Copyright (C) 1994-2019 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of the PBS Professional ("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * For a copy of the commercial license terms and conditions,
 * go to: (http://www.pbspro.com/UserArea/agreement.html)
 * or contact the Altair Legal Department.
 *
 * Altair’s dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of PBS Pro and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair’s trademarks, including but not limited to "PBS™",
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 * trademark licensing policies.
 *
 */


/**
 * @file	dec_svrattrl.c
 * @brief
 * decode_DIS_svrattrl() - decode into a list of server "svrattrl" structures
 *
 *	The space for the svrattrl structures is allocated as needed.
 *
 *	The first item is a unsigned integer, a count of the
 *	number of svrattrl entries in the linked list.  This is encoded
 *	even when there are no entries in the list.
 *
 *	Each individual entry is encoded as:
 *		u int	size of the three strings (name, resource, value)
 *			including the terminating nulls
 *		string	attribute name
 *		u int	1 or 0 if resource name does or does not follow
 *		string	resource name (if one)
 *		string  value of attribute/resource
 *		u int	"op" of attrlop
 *
 * @note
 *	the encoding of a svrattrl is the same as the encoding of
 *	the pbs_ifl.h structures "attrl" and "attropl".  Any one of
 *	the three forms can be decoded into any of the three with the
 *	possible loss of the "flags" field (which is the "op" of the attrlop).
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "libpbs.h"
#include "list_link.h"
#include "attribute.h"
#include "dis.h"

/**
 * @brief-
 *	 decode into a list of server "svrattrl" structures
 *
 * @par	Functionality:
 *		The space for the svrattrl structures is allocated as needed.
 *
 *      The first item is a unsigned integer, a count of the
 *      number of svrattrl entries in the linked list.  This is encoded
 *      even when there are no entries in the list.
 *
 * @par	Each individual entry is encoded as:\n
 *			u int	- size of the three strings (name, resource, value)
 *                      	  including the terminating nulls\n
 *			string  - attribute name\n
 *			u int   - 1 or 0 if resource name does or does not follow\n
 *			string  - resource name (if one)\n
 *			string  - value of attribute/resource\n
 *			u int   - "op" of attrlop\n
 *
 * NOTE:
 *	the encoding of a svrattrl is the same as the encoding of
 *      the pbs_ifl.h structures "attrl" and "attropl".  Any one of
 *      the three forms can be decoded into any of the three with the
 *      possible loss of the "flags" field (which is the "op" of the attrlop).
 *
 * @param[in] sock - socket descriptor
 * @param[in] phead - head pointer to list entry list sub-structure
 *
 * @return      int
 * @retval      DIS_SUCCESS(0)  success
 * @retval      error code      error
 *
 */

int
decode_DIS_svrattrl(int sock, pbs_list_head *phead)
{
	int		i;
	unsigned int	hasresc;
	size_t		ls;
	unsigned int	data_len;
	unsigned int	numattr;
	svrattrl       *psvrat;
	int		rc;
	size_t		tsize;


	numattr = disrui(sock, &rc);	/* number of attributes in set */
	if (rc) return rc;

	for (i=0; i<numattr; ++i) {

		data_len = disrui(sock, &rc);	/* here it is used */
		if (rc) return rc;

		tsize = sizeof(svrattrl) + data_len;
		if ((psvrat = (svrattrl *)malloc(tsize)) == 0)
			return DIS_NOMALLOC;

		CLEAR_LINK(psvrat->al_link);
		psvrat->al_sister = NULL;
		psvrat->al_atopl.next = 0;
		psvrat->al_tsize = tsize;
		psvrat->al_name  = (char *)psvrat + sizeof(svrattrl);
		psvrat->al_resc  = 0;
		psvrat->al_value = 0;
		psvrat->al_nameln = 0;
		psvrat->al_rescln = 0;
		psvrat->al_valln  = 0;
		psvrat->al_flags  = 0;
		psvrat->al_refct  = 1;

		if ((rc = disrfcs(sock, &ls, data_len, psvrat->al_name)) != 0)
			break;
		*(psvrat->al_name + ls++) = '\0';
		psvrat->al_nameln = (int)ls;
		data_len -= ls;

		hasresc = disrui(sock, &rc);
		if (rc) break;
		if (hasresc) {
			psvrat->al_resc = psvrat->al_name + ls;
			rc = disrfcs(sock, &ls, data_len, psvrat->al_resc);
			if (rc)
				break;
			*(psvrat->al_resc + ls++) = '\0';
			psvrat->al_rescln = (int)ls;
			data_len -= ls;
		}

		psvrat->al_value  = psvrat->al_name + psvrat->al_nameln +
			psvrat->al_rescln;
		if ((rc = disrfcs(sock, &ls, data_len, psvrat->al_value)) != 0)
			break;
		*(psvrat->al_value + ls++) = '\0';
		psvrat->al_valln = (int)ls;

		psvrat->al_op = (enum batch_op)disrui(sock, &rc);
		if (rc) break;

		append_link(phead, &psvrat->al_link, psvrat);
	}

	if (rc) {
		(void)free(psvrat);
	}

	return (rc);
}
