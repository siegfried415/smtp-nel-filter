#ifndef TIME_H
#define TIME_H

/*
  smtp_date_time is a date
  - day is the day of month (1 to 31)
  - month (1 to 12)
  - year (4 digits)
  - hour (0 to 23)
  - min (0 to 59)
  - sec (0 to 59)
  - zone (this is the decimal value that we can read, for example:
    for "-0200", the value is -200)
*/
struct smtp_date_time
{
	int dt_day;
	int dt_month;
	int dt_year;
	int dt_hour;
	int dt_min;
	int dt_sec;
	int dt_zone;
};

struct smtp_date_time *smtp_date_time_new (int dt_day, int dt_month,
					   int dt_year, int dt_hour,
					   int dt_min, int dt_sec,
					   int dt_zone);

void smtp_date_time_free (struct smtp_date_time *date_time);


/*
  smtp_date_time_parse will parse the given RFC 2822 date
  
  @param message this is a string containing the date
  @param length this is the size of the given string
  @param index this is a pointer to the start of the date in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

int smtp_date_time_parse (const char *message, size_t length,
			  size_t * index, struct smtp_date_time **result);

/*
  smtp_orig_date is the parsed Date field
  - date_time is the parsed date
*/
struct smtp_orig_date
{
	struct smtp_date_time *dt_date_time;	/* != NULL */
};


struct smtp_orig_date *smtp_orig_date_new (struct smtp_date_time
					   *dt_date_time);

void smtp_orig_date_free (struct smtp_orig_date *orig_date);

int
smtp_day_of_week_parse (const char *message, size_t length,
                        size_t * index, int *result);


int
smtp_date_parse (const char *message, size_t length,
                 size_t * index, int *pday, int *pmonth, int *pyear);

int
smtp_time_parse (const char *message, size_t length,
                 size_t * index, int *phour, int *pmin, int *psec, int *pzone);

int
smtp_day_name_parse (const char *message, size_t length,
                     size_t * index, int *result);

int
smtp_day_parse (const char *message, size_t length,
			size_t * index, int *result);

int
smtp_month_parse (const char *message, size_t length,
                  size_t * index, int *result);


int
smtp_year_parse (const char *message, size_t length,
                 size_t * index, int *result);

int
smtp_month_name_parse (const char *message, size_t length,
                       size_t * index, int *result);


int
smtp_time_of_day_parse (const char *message, size_t length,
	size_t * index, int *phour, int *pmin, int *psec);

int
smtp_zone_parse (const char *message, size_t length,
                 size_t * index, int *result);

int
smtp_hour_parse (const char *message, size_t length,
                 size_t * index, int *result);

int
smtp_minute_parse (const char *message, size_t length,
                   size_t * index, int *result);

int
smtp_second_parse (const char *message, size_t length,
                   size_t * index, int *result);


#endif
