


/*
 * $Id: time.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "_time.h"

struct smtp_date_time *
smtp_date_time_new (int dt_day, int dt_month, int dt_year,
		    int dt_hour, int dt_min, int dt_sec, int dt_zone)
{
	struct smtp_date_time *date_time;

	date_time = malloc (sizeof (*date_time));
	if (date_time == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_date_time_new: MALLOC pointer=%p\n",
		    date_time);

	date_time->dt_day = dt_day;
	date_time->dt_month = dt_month;
	date_time->dt_year = dt_year;
	date_time->dt_hour = dt_hour;
	date_time->dt_min = dt_min;
	date_time->dt_sec = dt_sec;
	date_time->dt_zone = dt_zone;

	return date_time;
}


void
smtp_date_time_free (struct smtp_date_time *date_time)
{
	free (date_time);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_date_time_free: FREE pointer=%p\n",
		    date_time);
}

/*
date-time       =       [ day-of-week "," ] date FWS time [CFWS]
*/

int
smtp_date_time_parse (const char *message, size_t length,
		      size_t * index, struct smtp_date_time **result)
{
	size_t cur_token;
	int day_of_week;
	struct smtp_date_time *date_time;
	int day;
	int month;
	int year;
	int hour;
	int min;
	int sec;
	int zone;
	int r;

	cur_token = *index;

	day_of_week = -1;
	r = smtp_day_of_week_parse (message, length, &cur_token,
				    &day_of_week);
	if (r == SMTP_NO_ERROR) {
		r = smtp_comma_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR)
			return r;
	}
	else if (r != SMTP_ERROR_PARSE)
		return r;

	day = 0;
	month = 0;
	year = 0;
	r = smtp_date_parse (message, length, &cur_token, &day, &month,
			     &year);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_fws_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	hour = 0;
	min = 0;
	sec = 0;
	zone = 0;
	r = smtp_time_parse (message, length, &cur_token,
			     &hour, &min, &sec, &zone);
	if (r != SMTP_NO_ERROR)
		return r;

	date_time =
		smtp_date_time_new (day, month, year, hour, min, sec, zone);
	if (date_time == NULL)
		return SMTP_ERROR_MEMORY;

	*index = cur_token;
	*result = date_time;

	return SMTP_NO_ERROR;
}

/*
day-of-week     =       ([FWS] day-name) / obs-day-of-week
*/

int
smtp_day_of_week_parse (const char *message, size_t length,
			size_t * index, int *result)
{
	size_t cur_token;
	int day_of_week;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_day_name_parse (message, length, &cur_token, &day_of_week);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = day_of_week;

	return SMTP_NO_ERROR;
}

/*
day-name        =       "Mon" / "Tue" / "Wed" / "Thu" /
                        "Fri" / "Sat" / "Sun"
*/

struct smtp_token_value
{
	int value;
	char *str;
};

struct smtp_token_value day_names[] = {
	{1, "Mon"},
	{2, "Tue"},
	{3, "Wed"},
	{4, "Thu"},
	{5, "Fri"},
	{6, "Sat"},
	{7, "Sun"},
};

enum
{
	DAY_NAME_START,
	DAY_NAME_T,
	DAY_NAME_S
};

int
guess_day_name (const char *message, size_t length, size_t index)
{
	int state;

	state = DAY_NAME_START;

	while (1) {

		if (index >= length)
			return -1;

		switch (state) {
		case DAY_NAME_START:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'M':	/* Mon */
				return 1;
				break;
			case 'T':	/* Tue Thu */
				state = DAY_NAME_T;
				break;
			case 'W':	/* Wed */
				return 3;
			case 'F':
				return 5;
			case 'S':	/* Sat Sun */
				state = DAY_NAME_S;
				break;
			default:
				return -1;
			}
			break;
		case DAY_NAME_T:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'U':
				return 2;
			case 'H':
				return 4;
			default:
				return -1;
			}
			break;
		case DAY_NAME_S:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'A':
				return 6;
			case 'U':
				return 7;
			default:
				return -1;
			}
			break;
		}

		index++;
	}
}

int
smtp_day_name_parse (const char *message, size_t length,
		     size_t * index, int *result)
{
	size_t cur_token;
	int day_of_week;
	int guessed_day;
	int r;

	cur_token = *index;

	guessed_day = guess_day_name (message, length, cur_token);
	if (guessed_day == -1)
		return SMTP_ERROR_PARSE;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token,
					       day_names[guessed_day -
							 1].str);
	if (r != SMTP_NO_ERROR)
		return r;

	day_of_week = guessed_day;

	*result = day_of_week;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
date            =       day month year
*/

int
smtp_date_parse (const char *message, size_t length,
		 size_t * index, int *pday, int *pmonth, int *pyear)
{
	size_t cur_token;
	int day;
	int month;
	int year;
	int r;

	cur_token = *index;

	day = 1;
	r = smtp_day_parse (message, length, &cur_token, &day);
	if (r != SMTP_NO_ERROR)
		return r;

	month = 1;
	r = smtp_month_parse (message, length, &cur_token, &month);
	if (r != SMTP_NO_ERROR)
		return r;

	year = 2001;
	r = smtp_year_parse (message, length, &cur_token, &year);
	if (r != SMTP_NO_ERROR)
		return r;

	*pday = day;
	*pmonth = month;
	*pyear = year;

	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
year            =       4*DIGIT / obs-year
*/

int
smtp_year_parse (const char *message, size_t length,
		 size_t * index, int *result)
{
	uint32_t number;
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_number_parse (message, length, &cur_token, &number);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	*result = number;

	return SMTP_NO_ERROR;
}

/*
month           =       (FWS month-name FWS) / obs-month
*/

int
smtp_month_parse (const char *message, size_t length,
		  size_t * index, int *result)
{
	size_t cur_token;
	int month;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_month_name_parse (message, length, &cur_token, &month);
	if (r != SMTP_NO_ERROR)
		return r;

	*result = month;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
month-name      =       "Jan" / "Feb" / "Mar" / "Apr" /
                        "May" / "Jun" / "Jul" / "Aug" /
                        "Sep" / "Oct" / "Nov" / "Dec"
*/

struct smtp_token_value month_names[] = {
	{1, "Jan"},
	{2, "Feb"},
	{3, "Mar"},
	{4, "Apr"},
	{5, "May"},
	{6, "Jun"},
	{7, "Jul"},
	{8, "Aug"},
	{9, "Sep"},
	{10, "Oct"},
	{11, "Nov"},
	{12, "Dec"},
};

enum
{
	MONTH_START,
	MONTH_J,
	MONTH_JU,
	MONTH_M,
	MONTH_MA,
	MONTH_A
};

int
guess_month (const char *message, size_t length, size_t index)
{
	int state;

	state = MONTH_START;

	while (1) {

		if (index >= length)
			return -1;

		switch (state) {
		case MONTH_START:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'J':	/* Jan Jun Jul */
				state = MONTH_J;
				break;
			case 'F':	/* Feb */
				return 2;
			case 'M':	/* Mar May */
				state = MONTH_M;
				break;
			case 'A':	/* Apr Aug */
				state = MONTH_A;
				break;
			case 'S':	/* Sep */
				return 9;
			case 'O':	/* Oct */
				return 10;
			case 'N':	/* Nov */
				return 11;
			case 'D':	/* Dec */
				return 12;
			default:
				return -1;
			}
			break;
		case MONTH_J:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'A':
				return 1;
			case 'U':
				state = MONTH_JU;
				break;
			default:
				return -1;
			}
			break;
		case MONTH_JU:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'N':
				return 6;
			case 'L':
				return 7;
			default:
				return -1;
			}
			break;
		case MONTH_M:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'A':
				state = MONTH_MA;
				break;
			default:
				return -1;
			}
			break;
		case MONTH_MA:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'Y':
				return 5;
			case 'R':
				return 3;
			default:
				return -1;
			}
			break;
		case MONTH_A:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'P':
				return 4;
			case 'U':
				return 8;
			default:
				return -1;
			}
			break;
		}

		index++;
	}
}

int
smtp_month_name_parse (const char *message, size_t length,
		       size_t * index, int *result)
{
	size_t cur_token;
	int month;
	int guessed_month;
	int r;

	cur_token = *index;

	guessed_month = guess_month (message, length, cur_token);
	if (guessed_month == -1)
		return SMTP_ERROR_PARSE;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token,
					       month_names[guessed_month -
							   1].str);
	if (r != SMTP_NO_ERROR)
		return r;

	month = guessed_month;

	*result = month;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
day             =       ([FWS] 1*2DIGIT) / obs-day
*/

int
smtp_day_parse (const char *message, size_t length,
		size_t * index, int *result)
{
	size_t cur_token;
	uint32_t day;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_number_parse (message, length, &cur_token, &day);
	if (r != SMTP_NO_ERROR)
		return r;

	*result = day;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
time            =       time-of-day FWS zone
*/

int
smtp_time_parse (const char *message, size_t length,
		 size_t * index, int *phour, int *pmin, int *psec, int *pzone)
{
	size_t cur_token;
	int hour;
	int min;
	int sec;
	int zone;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_time_of_day_parse (message, length, &cur_token,
				    &hour, &min, &sec);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_zone_parse (message, length, &cur_token, &zone);
	if (r == SMTP_NO_ERROR) {
		/* do nothing */
	}
	else if (r == SMTP_ERROR_PARSE) {
		zone = 0;
	}
	else {
		return r;
	}

	*phour = hour;
	*pmin = min;
	*psec = sec;
	*pzone = zone;

	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
time-of-day     =       hour ":" minute [ ":" second ]
*/

int
smtp_time_of_day_parse (const char *message, size_t length,
			size_t * index, int *phour, int *pmin, int *psec)
{
	int hour;
	int min;
	int sec;
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_hour_parse (message, length, &cur_token, &hour);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_minute_parse (message, length, &cur_token, &min);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_colon_parse (message, length, &cur_token);
	if (r == SMTP_NO_ERROR) {
		r = smtp_second_parse (message, length, &cur_token, &sec);
		if (r != SMTP_NO_ERROR)
			return r;
	}
	else if (r == SMTP_ERROR_PARSE)
		sec = 0;
	else
		return r;

	*phour = hour;
	*pmin = min;
	*psec = sec;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
hour            =       2DIGIT / obs-hour
*/

int
smtp_hour_parse (const char *message, size_t length,
		 size_t * index, int *result)
{
	uint32_t hour;
	int r;

	r = smtp_number_parse (message, length, index, &hour);
	if (r != SMTP_NO_ERROR)
		return r;

	*result = hour;

	return SMTP_NO_ERROR;
}

/*
minute          =       2DIGIT / obs-minute
*/

int
smtp_minute_parse (const char *message, size_t length,
		   size_t * index, int *result)
{
	uint32_t minute;
	int r;

	r = smtp_number_parse (message, length, index, &minute);
	if (r != SMTP_NO_ERROR)
		return r;

	*result = minute;

	return SMTP_NO_ERROR;
}

/*
second          =       2DIGIT / obs-second
*/

int
smtp_second_parse (const char *message, size_t length,
		   size_t * index, int *result)
{
	uint32_t second;
	int r;

	r = smtp_number_parse (message, length, index, &second);
	if (r != SMTP_NO_ERROR)
		return r;

	*result = second;

	return SMTP_NO_ERROR;
}

/*
zone            =       (( "+" / "-" ) 4DIGIT) / obs-zone
*/

/*
obs-zone        =       "UT" / "GMT" /          ; Universal Time
                                                ; North American UT
                                                ; offsets
                        "EST" / "EDT" /         ; Eastern:  - 5/ - 4
                        "CST" / "CDT" /         ; Central:  - 6/ - 5
                        "MST" / "MDT" /         ; Mountain: - 7/ - 6
                        "PST" / "PDT" /         ; Pacific:  - 8/ - 7

                        %d65-73 /               ; Military zones - "A"
                        %d75-90 /               ; through "I" and "K"
                        %d97-105 /              ; through "Z", both
                        %d107-122               ; upper and lower case
*/

enum
{
	STATE_ZONE_1 = 0,
	STATE_ZONE_2 = 1,
	STATE_ZONE_3 = 2,
	STATE_ZONE_OK = 3,
	STATE_ZONE_ERR = 4,
	STATE_ZONE_CONT = 5,
};

int
smtp_zone_parse (const char *message, size_t length,
		 size_t * index, int *result)
{
	uint32_t zone;
	int sign;
	size_t cur_token;
	int r;

	cur_token = *index;

	if (cur_token + 1 < length) {
		if ((message[cur_token] == 'U')
		    && (message[cur_token] == 'T')) {
			*result = TRUE;
			*index = cur_token + 2;

			return SMTP_NO_ERROR;
		}
	}

	if (cur_token + 2 < length) {
		int state;

		state = STATE_ZONE_1;

		while (state <= 2) {
			switch (state) {
			case STATE_ZONE_1:
				switch (message[cur_token]) {
				case 'G':
					if (message[cur_token + 1] == 'M'
					    && message[cur_token + 2] ==
					    'T') {
						zone = 0;
						state = STATE_ZONE_OK;
					}
					else {
						state = STATE_ZONE_ERR;
					}
					break;
				case 'E':
					zone = -5;
					state = STATE_ZONE_2;
					break;
				case 'C':
					zone = -6;
					state = STATE_ZONE_2;
					break;
				case 'M':
					zone = -7;
					state = STATE_ZONE_2;
					break;
				case 'P':
					zone = -8;
					state = STATE_ZONE_2;
					break;
				default:
					state = STATE_ZONE_CONT;
					break;
				}
				break;
			case STATE_ZONE_2:
				switch (message[cur_token + 1]) {
				case 'S':
					state = STATE_ZONE_3;
					break;
				case 'D':
					zone++;
					state = STATE_ZONE_3;
					break;
				default:
					state = STATE_ZONE_ERR;
					break;
				}
				break;
			case STATE_ZONE_3:
				if (message[cur_token + 2] == 'T') {
					zone *= 100;
					state = STATE_ZONE_OK;
				}
				else
					state = STATE_ZONE_ERR;
				break;
			}
		}

		switch (state) {
		case STATE_ZONE_OK:
			*result = zone;
			*index = cur_token + 3;
			return SMTP_NO_ERROR;

		case STATE_ZONE_ERR:
			return SMTP_ERROR_PARSE;
		}
	}

	sign = 1;
	r = smtp_plus_parse (message, length, &cur_token);
	if (r == SMTP_NO_ERROR)
		sign = 1;

	if (r == SMTP_ERROR_PARSE) {
		r = smtp_minus_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR)
			sign = -1;
	}

	if (r == SMTP_NO_ERROR) {
		/* do nothing */
	}
	else if (r == SMTP_ERROR_PARSE)
		sign = 1;
	else
		return r;

	r = smtp_number_parse (message, length, &cur_token, &zone);
	if (r != SMTP_NO_ERROR)
		return r;

	zone = zone * sign;

	*index = cur_token;
	*result = zone;

	return SMTP_NO_ERROR;
}

/*
orig-date       =       "Date:" date-time CRLF
*/


int
smtp_orig_date_parse (const char *message, size_t length,
		      size_t * index, struct smtp_orig_date **result)
{
	struct smtp_date_time *date_time;
	struct smtp_orig_date *orig_date;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

#if 0
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Date:");
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}
#endif


	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_date_time_parse (message, length, &cur_token, &date_time);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_ignore_unstructured_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_date_time;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_date_time;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	orig_date = smtp_orig_date_new (date_time);
	if (orig_date == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_date_time;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*result = orig_date;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_date_time:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_date_time_free (date_time);
      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;
}


struct smtp_orig_date *
smtp_orig_date_new (struct smtp_date_time *dt_date_time)
{
	struct smtp_orig_date *orig_date;

	orig_date = malloc (sizeof (*orig_date));
	if (orig_date == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_orig_date_new: MALLOC pointer=%p\n",
		    orig_date);

	orig_date->dt_date_time = dt_date_time;

	return orig_date;
}


void
smtp_orig_date_free (struct smtp_orig_date *orig_date)
{
	if (orig_date->dt_date_time != NULL)
		smtp_date_time_free (orig_date->dt_date_time);
	free (orig_date);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_orig_date_free: FREE pointer=%p\n",
		    orig_date);
}
