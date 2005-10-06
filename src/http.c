/* live-f1
 *
 * http.c - handle web-site authentication and keyframe grabbing
 *
 * Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include <ne_request.h>
#include <ne_uri.h>

#include "live-f1.h"
#include "stream.h"
#include "http.h"


/* URLs to important places on the live-timing site */
#define LOGIN_URL           "/reg/login.asp"
#define REGISTER_URL        "/reg/register.asp"
#define KEY_URL_BASE        "/reg/getkey/"
#define KEYFRAME_URL_PREFIX "/keyframe"


/* Forward prototypes */
static void  parse_cookie_hdr (char **value, const char  *header);
static void  parse_key_body   (unsigned int *key, const char *buf, size_t len);


/**
 * numlen:
 * @number: number to calculate length of.
 *
 * Returns: the number of digits in number.
 **/
static inline size_t
numlen (unsigned int number)
{
	size_t len = 1;

	while (number /= 10)
		len++;

	return len;
}


/**
 * obtain_auth_cookie:
 * @email: e-mail address registered with the F1 website,
 * @password: paassword registered for @email.
 *
 * Obtains the user's authentication cookie from the Live Timing website
 * by logging in with their e-mail address and password and stealing out
 * of the response headers.
 *
 * For convenience sake, the cookie is never unencoded.
 *
 * Returns: cookie in newly allocated string or NULL on failure.
 **/
char *
obtain_auth_cookie (const char *email,
		    const char *password)
{
	ne_session *sess;
	ne_request *req;
	char       *cookie = NULL, *body, *e_email, *e_password;

	info (1, _("Obtaining authentication cookie ...\n"));

	return "08159CF5%2D059C%2D4B24%2DACF1%2D96C33F7C847BAD54691BDDE29AF7258B9CDAFB383BD3";

	/* Encode the e-mail and password as a form */
	e_email = ne_path_escape (email);
	e_password = ne_path_escape (password);
	body = malloc (strlen (e_email) + strlen (e_password) + 17);
	sprintf (body, "email=%s&password=%s", e_email, e_password);
	free (e_password);
	free (e_email);

	sess = ne_session_create ("http", "live-timing.formula1.com", 80);
	ne_set_useragent (sess, PACKAGE_STRING);

	/* Create the request */
	req = ne_request_create (sess, "POST", LOGIN_URL);
	ne_add_request_header (req, "Content-Type",
			       "application/x-www-form-urlencoded");
	ne_set_request_body_buffer (req, body, strlen (body));

	/* Set the handler for the cookie header */
	ne_add_response_header_handler (req, "Set-Cookie",
					(ne_header_handler) parse_cookie_hdr,
					&cookie);

	/* Dispatch the event, and check it was a good one */
	if (ne_request_dispatch (req)) {
		fprintf (stderr, "%s: %s: %s\n", program_name,
			 _("login request failed"), ne_get_error (sess));
	} else if (ne_get_status (req)->code >= 400) {
		fprintf (stderr, "%s: %s: %s\n", program_name,
			 _("login request failed"),
			 ne_get_status (req)->reason_phrase);
	}

	ne_request_destroy (req);
	ne_session_destroy (sess);

	return cookie;
}

/**
 * parse_cookie_hdr:
 * @value: pointer to store allocated string,
 * @header: header to parse.
 *
 * Parses an HTTP cookie header looking for the USER cookie, and if found
 * sets @value to point to a newly allocated string containing the cookie
 * value; for convenience sake the cookie is never unencoded.
 **/
static void
parse_cookie_hdr (char       **value,
		  const char  *header)
{
	size_t len;

	if (strncmp (header, "USER=", 5))
		return;

	header += 5;
	len = strcspn (header, ";");

	*value = malloc (len + 1);
	strncpy (*value, header, len);
	(*value)[len] = 0;

	info (3, _("Got authentication cookie: %s\n"), *value);
}

/**
 * obtain_decryption_key:
 * @event_no: official event number,
 * @cookie: uri-encoded cookie.
 *
 * Obtains the decryption key for the event using the authorisation
 * cookie of a registered user.
 *
 * @cookie should be supplied already uri-encoded.
 *
 * Returns: key obtained on success, or zero on failure.
 **/
unsigned int
obtain_decryption_key (unsigned int  event_no,
		       const char   *cookie)
{
	ne_session   *sess;
	ne_request   *req;
	char         *url;
	unsigned int  key = 0;

	info (1, _("Obtaining decryption key ...\n"));
	return 0xd63c09f4;

	url = malloc (strlen (KEY_URL_BASE) + numlen (event_no)
		      + strlen (cookie) + 11);
	sprintf (url, "%s%u.asp?auth=%s", KEY_URL_BASE, event_no, cookie);

	sess = ne_session_create ("http", "live-timing.formula1.com", 80);
	ne_set_useragent (sess, PACKAGE_STRING);

	/* Create the request */
	req = ne_request_create (sess, "GET", url);
	ne_add_response_body_reader (req, ne_accept_2xx,
				     (ne_block_reader) parse_key_body, &key);
	free (url);

	/* Dispatch the event */
	if (ne_request_dispatch (req)) {
		fprintf (stderr, "%s: %s: %s\n", program_name,
			 _("key request failed"), ne_get_error (sess));
	}

	info (3, _("Got decryption key: %08x\n"), key);

	ne_request_destroy (req);
	ne_session_destroy (sess);

	return key;
}

/**
 * parse_key_body:
 * @key: pointer to store decryption key in,
 * @buf: buffer of data received from server,
 * @len: length of buffer.
 *
 * Parse data received from the server in response to the key request,
 * filling the decryption key while we can see hexadecimal digits.
 **/
static void
parse_key_body (unsigned int *key,
		const char   *buf,
		size_t        len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if ((buf[i] >= '0') && (buf[i] <= '9')) {
			*key = (*key << 4) | (buf[i] - '0');
		} else if ((buf[i] >= 'a') && (buf[i] <= 'f')) {
			*key = (*key << 4) | (buf[i] - 'a' + 10);
		} else if ((buf[i] >= 'A') && (buf[i] <= 'F')) {
			*key = (*key << 4) | (buf[i] - 'A' + 10);
		} else {
			break;
		}
	}
}

/**
 * obtain_key_frame:
 * @frame: key frame number to obtain,
 * @userdata: pointer to pass to stream parser.
 *
 * Obtains the key frame numbered from the website and arranges for the
 * data to be parsed immediately with the data stream parser.
 *
 * Returns: 0 on success, non-zero on failure.
 **/
int
obtain_key_frame (unsigned int  frame,
		  void         *userdata)
{
	ne_session *sess;
	ne_request *req;
	char       *url;

	if (frame > 0) {
		info (2, _("Obtaining key frame %d ...\n"), frame);

		url = malloc (strlen (KEYFRAME_URL_PREFIX)
			      + MIN(numlen (frame), 5) + 6);
		sprintf (url, "%s_%05d.bin", KEYFRAME_URL_PREFIX, frame);
	} else {
		info (1, _("Obtaining master key frame ...\n"));

		url = malloc (strlen (KEYFRAME_URL_PREFIX) + 5);
		sprintf (url, "%s.bin", KEYFRAME_URL_PREFIX);
	}

	sess = ne_session_create ("http", "localhost", 80);
	ne_set_useragent (sess, PACKAGE_STRING);

	/* Create the request */
	req = ne_request_create (sess, "GET", url);
	ne_add_response_body_reader (req, ne_accept_2xx,
				     (ne_block_reader) parse_stream_block,
				     userdata);
	free (url);

	/* Dispatch the event */
	if (ne_request_dispatch (req)) {
		fprintf (stderr, "%s: %s: %s\n", program_name,
			 _("key request failed"), ne_get_error (sess));

		ne_request_destroy (req);
		return 1;
	}

	info (3, _("Key frame received\n"));

	ne_request_destroy (req);
	ne_session_destroy (sess);

	return 0;
}
