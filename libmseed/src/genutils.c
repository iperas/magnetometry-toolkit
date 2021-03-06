/***************************************************************************
 * genutils.c
 *
 * Generic utility routines
 *
 * Written by Chad Trabant
 * ORFEUS/EC-Project MEREDIAN
 * IRIS Data Management Center
 *
 * modified: 2007.228
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libmseed.h"

static hptime_t ms_time2hptime_int (int year, int day, int hour,
				    int min, int sec, int usec);


/***************************************************************************
 * ms_recsrcname:
 *
 * Generate a source name string for a specified raw data record in
 * the format: 'NET_STA_LOC_CHAN' or, if the quality flag is true:
 * 'NET_STA_LOC_CHAN_QUAL'.  The passed srcname must have enough room
 * for the resulting string.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_recsrcname (char *record, char *srcname, flag quality)
{
  struct fsdh_s *fsdh;
  char network[6];
  char station[6];
  char location[6];
  char channel[6];
  
  if ( ! record )
    return NULL;
  
  fsdh = (struct fsdh_s *) record;
  
  ms_strncpclean (network, fsdh->network, 2);
  ms_strncpclean (station, fsdh->station, 5);
  ms_strncpclean (location, fsdh->location, 2);
  ms_strncpclean (channel, fsdh->channel, 3);
  
  /* Build the source name string including the quality indicator*/
  if ( quality )
    sprintf (srcname, "%s_%s_%s_%s_%c",
             network, station, location, channel, fsdh->dataquality);
  
  /* Build the source name string without the quality indicator*/
  else
    sprintf (srcname, "%s_%s_%s_%s", network, station, location, channel);
  
  return srcname;
} /* End of ms_recsrcname() */


/***************************************************************************
 * ms_strncpclean:
 *
 * Copy up to 'length' characters from 'source' to 'dest' while
 * removing all spaces.  The result is left justified and always null
 * terminated.  The destination string must have enough room needed
 * for the non-space characters within 'length' and the null
 * terminator, a maximum of 'length + 1'.
 * 
 * Returns the number of characters (not including the null terminator) in
 * the destination string.
 ***************************************************************************/
int
ms_strncpclean (char *dest, const char *source, int length)
{
  int sidx, didx;
  
  if ( ! dest )
    return 0;
  
  if ( ! source )
    {
      *dest = '\0';
      return 0;
    }

  for ( sidx=0, didx=0; sidx < length ; sidx++ )
    {
      if ( *(source+sidx) == '\0' )
	{
	  break;
	}

      if ( *(source+sidx) != ' ' )
	{
	  *(dest+didx) = *(source+sidx);
	  didx++;
	}
    }

  *(dest+didx) = '\0';
  
  return didx;
}  /* End of ms_strncpclean() */


/***************************************************************************
 * ms_strncpopen:
 *
 * Copy 'length' characters from 'source' to 'dest', padding the right
 * side with spaces and leave open-ended.  The result is left
 * justified and *never* null terminated (the open-ended part).  The
 * destination string must have enough room for 'length' characters.
 * 
 * Returns the number of characters copied from the source string.
 ***************************************************************************/
int
ms_strncpopen (char *dest, const char *source, int length)
{
  int didx;
  int dcnt = 0;
  int term = 0;
  
  if ( ! dest )
    return 0;
  
  if ( ! source )
    {
      for ( didx=0; didx < length ; didx++ )
	{
	  *(dest+didx) = ' ';
	}
      
      return 0;
    }
  
  for ( didx=0; didx < length ; didx++ )
    {
      if ( !term )
	if ( *(source+didx) == '\0' )
	  term = 1;
      
      if ( !term )
	{
	  *(dest+didx) = *(source+didx);
	  dcnt++;
	}
      else
	{
	  *(dest+didx) = ' ';
	}
    }
  
  return dcnt;
}  /* End of ms_strncpopen() */


/***************************************************************************
 * ms_doy2md:
 *
 * Compute the month and day-of-month from a year and day-of-year.
 *
 * Year is expected to be in the range 1900-2100, jday is expected to
 * be in the range 1-366, month will be in the range 1-12 and mday
 * will be in the range 1-31.
 *
 * Returns 0 on success and -1 on error.
 ***************************************************************************/
int
ms_doy2md(int year, int jday, int *month, int *mday)
{
  int idx;
  int leap;
  int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  /* Sanity check for the supplied year */
  if ( year < 1900 || year > 2100 )
    {
      ms_log (2, "ms_doy2md(): year (%d) is out of range\n", year);
      return -1;
    }
  
  /* Test for leap year */
  leap = ( ((year%4 == 0) && (year%100 != 0)) || (year%400 == 0) ) ? 1 : 0;

  /* Add a day to February if leap year */
  if ( leap )
    days[1]++;

  if (jday > 365+leap || jday <= 0)
    {
      ms_log (2, "ms_doy2md(): day-of-year (%d) is out of range\n", jday);
      return -1;
    }
    
  for ( idx=0; idx < 12; idx++ )
    {
      jday -= days[idx];

      if ( jday <= 0 )
	{
	  *month = idx + 1;
	  *mday = days[idx] + jday;
	  break;
	}
    }

  return 0;
}  /* End of ms_doy2md() */


/***************************************************************************
 * ms_md2doy:
 *
 * Compute the day-of-year from a year, month and day-of-month.
 *
 * Year is expected to be in the range 1900-2100, month is expected to
 * be in the range 1-12, mday is expected to be in the range 1-31 and
 * jday will be in the range 1-366.
 *
 * Returns 0 on success and -1 on error.
 ***************************************************************************/
int
ms_md2doy(int year, int month, int mday, int *jday)
{
  int idx;
  int leap;
  int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  /* Sanity check for the supplied parameters */
  if ( year < 1900 || year > 2100 )
    {
      ms_log (2, "ms_md2doy(): year (%d) is out of range\n", year);
      return -1;
    }
  if ( month < 1 || month > 12 )
    {
      ms_log (2, "ms_md2doy(): month (%d) is out of range\n", month);
      return -1;
    }
  if ( mday < 1 || mday > 31 )
    {
      ms_log (2, "ms_md2doy(): day-of-month (%d) is out of range\n", mday);
      return -1;
    }
  
  /* Test for leap year */
  leap = ( ((year%4 == 0) && (year%100 != 0)) || (year%400 == 0) ) ? 1 : 0;
  
  /* Add a day to February if leap year */
  if ( leap )
    days[1]++;
  
  /* Check that the day-of-month jives with specified month */
  if ( mday > days[month-1] )
    {
      ms_log (2, "ms_md2doy(): day-of-month (%d) is out of range for month %d\n",
	       mday, month);
      return -1;
    }

  *jday = 0;
  month--;
  
  for ( idx=0; idx < 12; idx++ )
    {
      if ( idx == month )
	{
	  *jday += mday;
	  break;
	}
      
      *jday += days[idx];
    }
  
  return 0;
}  /* End of ms_md2doy() */


/***************************************************************************
 * ms_btime2hptime:
 *
 * Convert a binary SEED time structure to a high precision epoch time
 * (1/HPTMODULUS second ticks from the epoch).  The algorithm used is
 * a specific version of a generalized function in GNU glibc.
 *
 * Returns a high precision epoch time on success and HPTERROR on
 * error.
 ***************************************************************************/
hptime_t
ms_btime2hptime (BTime *btime)
{
  hptime_t hptime;
  int shortyear;
  int a4, a100, a400;
  int intervening_leap_days;
  int days;
  
  if ( ! btime )
    return HPTERROR;
  
  shortyear = btime->year - 1900;

  a4 = (shortyear >> 2) + 475 - ! (shortyear & 3);
  a100 = a4 / 25 - (a4 % 25 < 0);
  a400 = a100 >> 2;
  intervening_leap_days = (a4 - 492) - (a100 - 19) + (a400 - 4);
  
  days = (365 * (shortyear - 70) + intervening_leap_days + (btime->day - 1));
  
  hptime = (hptime_t ) (60 * (60 * (24 * days + btime->hour) + btime->min) + btime->sec) * HPTMODULUS
    + (btime->fract * (HPTMODULUS / 10000));
    
  return hptime;
}  /* End of ms_btime2hptime() */


/***************************************************************************
 * ms_btime2isotimestr:
 *
 * Build a time string in ISO recommended format from a BTime struct.
 *
 * The provided isostimestr must have enough room for the resulting time
 * string of 25 characters, i.e. '2001-07-29T12:38:00.0000' + NULL.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_btime2isotimestr (BTime *btime, char *isotimestr)
{  
  int month = 0;
  int mday = 0;
  int ret;

  if ( ! isotimestr )
    return NULL;

  if ( ms_doy2md (btime->year, btime->day, &month, &mday) )
    {
      ms_log (2, "ms_btime2isotimestr(): Error converting year %d day %d\n",
	      btime->year, btime->day);
      return NULL;
    }
  
  ret = snprintf (isotimestr, 25, "%4d-%02d-%02dT%02d:%02d:%02d.%04d",
		  btime->year, month, mday,
		  btime->hour, btime->min, btime->sec, btime->fract);
  
  if ( ret != 24 )
    return NULL;
  else
    return isotimestr;
}  /* End of ms_btime2isotimestr() */


/***************************************************************************
 * ms_btime2mdtimestr:
 *
 * Build a time string in month-day format from a BTime struct.
 * 
 * The provided isostimestr must have enough room for the resulting time
 * string of 25 characters, i.e. '2001-07-29 12:38:00.0000' + NULL.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_btime2mdtimestr (BTime *btime, char *mdtimestr)
{ 
  int month = 0;
  int mday = 0;
  int ret;
  
  if ( ! mdtimestr )
    return NULL;
  
  if ( ms_doy2md (btime->year, btime->day, &month, &mday) )
    {
      ms_log (2, "ms_btime2mdtimestr(): Error converting year %d day %d\n",
              btime->year, btime->day);
      return NULL;
    }
  
  ret = snprintf (mdtimestr, 25, "%4d-%02d-%02d %02d:%02d:%02d.%04d",
                  btime->year, month, mday,
                  btime->hour, btime->min, btime->sec, btime->fract);

  if ( ret != 24 )
    return NULL;
  else
    return mdtimestr;
}  /* End of ms_btime2mdtimestr() */


/***************************************************************************
 * ms_btime2seedtimestr:
 *
 * Build a SEED time string from a BTime struct.
 *
 * The provided seedtimestr must have enough room for the resulting time
 * string of 23 characters, i.e. '2001,195,12:38:00.0000' + NULL.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_btime2seedtimestr (BTime *btime, char *seedtimestr)
{
  int ret;
  
  if ( ! seedtimestr )
    return NULL;
  
  ret = snprintf (seedtimestr, 23, "%4d,%03d,%02d:%02d:%02d.%04d",
		  btime->year, btime->day,
		  btime->hour, btime->min, btime->sec, btime->fract);
  
  if ( ret != 22 )
    return NULL;
  else
    return seedtimestr;
}  /* End of ms_btime2seedtimestr() */


/***************************************************************************
 * ms_hptime2btime:
 *
 * Convert a high precision epoch time to a SEED binary time
 * structure.  The microseconds beyond the 1/10000 second range are
 * truncated and *not* rounded, this is intentional and necessary.
 *
 * Returns 0 on success and -1 on error.
 ***************************************************************************/
int
ms_hptime2btime (hptime_t hptime, BTime *btime)
{
  struct tm *tm;
  int isec;
  int ifract;
  int bfract;
  time_t tsec;
  
  if ( btime == NULL )
    return -1;
  
  /* Reduce to Unix/POSIX epoch time and fractional seconds */
  isec = MS_HPTIME2EPOCH(hptime);
  ifract = hptime - ((hptime_t)isec * HPTMODULUS);
  
  /* BTime only has 1/10000 second precision */
  bfract = ifract / (HPTMODULUS / 10000);
  
  /* Adjust for negative epoch times, round back when needed */
  if ( hptime < 0 && ifract != 0 )
    {
      /* Isolate microseconds between 1e-4 and 1e-6 precision and adjust bfract if not zero */
      if ( ifract - bfract * (HPTMODULUS / 10000) )
	bfract -= 1;
      
      isec -= 1;
      bfract = 10000 - (-bfract);
    }

  tsec = (time_t) isec;
  if ( ! (tm = gmtime ( &tsec )) )
    return -1;
  
  btime->year   = tm->tm_year + 1900;
  btime->day    = tm->tm_yday + 1;
  btime->hour   = tm->tm_hour;
  btime->min    = tm->tm_min;
  btime->sec    = tm->tm_sec;
  btime->unused = 0;
  btime->fract  = (uint16_t) bfract;
  
  return 0;
}  /* End of ms_hptime2btime() */


/***************************************************************************
 * ms_hptime2isotimestr:
 *
 * Build a time string in ISO recommended format from a high precision
 * epoch time.
 *
 * The provided isostimestr must have enough room for the resulting time
 * string of 27 characters, i.e. '2001-07-29T12:38:00.000000' + NULL.
 *
 * The 'subseconds' flag controls whenther the sub second portion of the
 * time is included or not.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_hptime2isotimestr (hptime_t hptime, char *isotimestr, flag subseconds)
{
  struct tm *tm;
  int isec;
  int ifract;
  int ret;
  time_t tsec;

  if ( isotimestr == NULL )
    return NULL;

  /* Reduce to Unix/POSIX epoch time and fractional seconds */
  isec = MS_HPTIME2EPOCH(hptime);
  ifract = (hptime_t) hptime - (isec * HPTMODULUS);
  
  /* Adjust for negative epoch times */
  if ( hptime < 0 && ifract != 0 )
    {
      isec -= 1;
      ifract = HPTMODULUS - (-ifract);
    }

  tsec = (time_t) isec;
  if ( ! (tm = gmtime ( &tsec )) )
    return NULL;
  
  if ( subseconds )
    /* Assuming ifract has at least microsecond precision */
    ret = snprintf (isotimestr, 27, "%4d-%02d-%02dT%02d:%02d:%02d.%06d",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec, ifract);
  else
    ret = snprintf (isotimestr, 20, "%4d-%02d-%02dT%02d:%02d:%02d",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);

  if ( ret != 26 && ret != 19 )
    return NULL;
  else
    return isotimestr;
}  /* End of ms_hptime2isotimestr() */


/***************************************************************************
 * ms_hptime2mdtimestr:
 *
 * Build a time string in month-day format from a high precision
 * epoch time.
 *
 * The provided mdtimestr must have enough room for the resulting time
 * string of 27 characters, i.e. '2001-07-29 12:38:00.000000' + NULL.
 *
 * The 'subseconds' flag controls whenther the sub second portion of the
 * time is included or not.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_hptime2mdtimestr (hptime_t hptime, char *mdtimestr, flag subseconds)
{
  struct tm *tm;
  int isec;
  int ifract;
  int ret;
  time_t tsec;

  if ( mdtimestr == NULL )
    return NULL;

  /* Reduce to Unix/POSIX epoch time and fractional seconds */
  isec = MS_HPTIME2EPOCH(hptime);
  ifract = (hptime_t) hptime - (isec * HPTMODULUS);

  /* Adjust for negative epoch times */
  if ( hptime < 0 && ifract != 0 )
    {
      isec -= 1;
      ifract = HPTMODULUS - (-ifract);
    }

  tsec = (time_t) isec;
  if ( ! (tm = gmtime ( &tsec )) )
    return NULL;

  if ( subseconds )
    /* Assuming ifract has at least microsecond precision */
    ret = snprintf (mdtimestr, 27, "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec, ifract);
  else
    ret = snprintf (mdtimestr, 20, "%4d-%02d-%02d %02d:%02d:%02d",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);

  if ( ret != 26 && ret != 19 )
    return NULL;
  else
    return mdtimestr;
}  /* End of ms_hptime2mdtimestr() */


/***************************************************************************
 * ms_hptime2seedtimestr:
 *
 * Build a SEED time string from a high precision epoch time.
 *
 * The provided seedtimestr must have enough room for the resulting time
 * string of 25 characters, i.e. '2001,195,12:38:00.000000\n'.
 *
 * The 'subseconds' flag controls whenther the sub second portion of the
 * time is included or not.
 *
 * Returns a pointer to the resulting string or NULL on error.
 ***************************************************************************/
char *
ms_hptime2seedtimestr (hptime_t hptime, char *seedtimestr, flag subseconds)
{
  struct tm *tm;
  int isec;
  int ifract;
  int ret;
  time_t tsec;
  
  if ( seedtimestr == NULL )
    return NULL;
  
  /* Reduce to Unix/POSIX epoch time and fractional seconds */
  isec = MS_HPTIME2EPOCH(hptime);
  ifract = (hptime_t) hptime - (isec * HPTMODULUS);
  
  /* Adjust for negative epoch times */
  if ( hptime < 0 && ifract != 0 )
    {
      isec -= 1;
      ifract = HPTMODULUS - (-ifract);
    }

  tsec = (time_t) isec;
  if ( ! (tm = gmtime ( &tsec )) )
    return NULL;
  
  if ( subseconds )
    /* Assuming ifract has at least microsecond precision */
    ret = snprintf (seedtimestr, 25, "%4d,%03d,%02d:%02d:%02d.%06d",
		    tm->tm_year + 1900, tm->tm_yday + 1,
		    tm->tm_hour, tm->tm_min, tm->tm_sec, ifract);
  else
    /* Assuming ifract has at least microsecond precision */
    ret = snprintf (seedtimestr, 18, "%4d,%03d,%02d:%02d:%02d",
                    tm->tm_year + 1900, tm->tm_yday + 1,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);
  
  if ( ret != 24 && ret != 17 )
    return NULL;
  else
    return seedtimestr;
}  /* End of ms_hptime2seedtimestr() */


/***************************************************************************
 * ms_time2hptime_int:
 *
 * Convert specified time values to a high precision epoch time.  This
 * is an internal version which does no range checking, it is assumed
 * that checking the range for each value has already been done.
 *
 * Returns epoch time on success and HPTERROR on error.
 ***************************************************************************/
static hptime_t
ms_time2hptime_int (int year, int day, int hour, int min, int sec, int usec)
{
  BTime btime;
  hptime_t hptime;
  
  memset (&btime, 0, sizeof(BTime));
  btime.day = 1;
  
  /* Convert integer seconds using ms_btime2hptime */
  btime.year = (int16_t) year;
  btime.day = (int16_t) day;
  btime.hour = (uint8_t) hour;
  btime.min = (uint8_t) min;
  btime.sec = (uint8_t) sec;
  btime.fract = 0;

  hptime = ms_btime2hptime (&btime);
  
  if ( hptime == HPTERROR )
    {
      ms_log (2, "ms_time2hptime(): Error converting with ms_btime2hptime()\n");
      return HPTERROR;
    }
  
  /* Add the microseconds */
  hptime += (hptime_t) usec * (1000000 / HPTMODULUS);
  
  return hptime;
}  /* End of ms_time2hptime_int() */


/***************************************************************************
 * ms_time2hptime:
 *
 * Convert specified time values to a high precision epoch time.  This
 * is essentially a frontend for ms_time2hptime that does range
 * checking for each input value.
 *
 * Expected ranges:
 * year : 1900 - 2100
 * day  : 1 - 366
 * hour : 0 - 23
 * min  : 0 - 59
 * sec  : 0 - 60
 * usec : 0 - 999999
 *
 * Returns epoch time on success and HPTERROR on error.
 ***************************************************************************/
hptime_t
ms_time2hptime (int year, int day, int hour, int min, int sec, int usec)
{
  if ( year < 1900 || year > 2100 )
    {
      ms_log (2, "ms_time2hptime(): Error with year value: %d\n", year);
      return HPTERROR;
    }
  
  if ( day < 1 || day > 366 )
    {
      ms_log (2, "ms_time2hptime(): Error with day value: %d\n", day);
      return HPTERROR;
    }
  
  if ( hour < 0 || hour > 23 )
    {
      ms_log (2, "ms_time2hptime(): Error with hour value: %d\n", hour);
      return HPTERROR;
    }
  
  if ( min < 0 || min > 59 )
    {
      ms_log (2, "ms_time2hptime(): Error with minute value: %d\n", min);
      return HPTERROR;
    }
  
  if ( sec < 0 || sec > 60 )
    {
      ms_log (2, "ms_time2hptime(): Error with second value: %d\n", sec);
      return HPTERROR;
    }
  
  if ( usec < 0 || usec > 999999 )
    {
      ms_log (2, "ms_time2hptime(): Error with microsecond value: %d\n", usec);
      return HPTERROR;
    }
  
  return ms_time2hptime_int (year, day, hour, min, sec, usec);
}  /* End of ms_time2hptime() */


/***************************************************************************
 * ms_seedtimestr2hptime:
 * 
 * Convert a SEED time string to a high precision epoch time.  SEED
 * time format is "YYYY[,DDD,HH,MM,SS.FFFFFF]", the delimiter can be a
 * comma [,], colon [:] or period [.] except for the fractional
 * seconds which must start with a period [.].
 *
 * The time string can be "short" in which case the omitted values are
 * assumed to be zero (with the exception of DDD which is assumed to
 * be 1): "YYYY,DDD,HH" assumes MM, SS and FFFF are 0.  The year is
 * required, otherwise there wouldn't be much for a date.
 *
 * Ranges are checked for each value.
 *
 * Returns epoch time on success and HPTERROR on error.
 ***************************************************************************/
hptime_t
ms_seedtimestr2hptime (char *seedtimestr)
{
  int fields;
  int year = 0;
  int day  = 1;
  int hour = 0;
  int min  = 0;
  int sec  = 0;
  float fusec = 0.0;
  int usec = 0;
  
  fields = sscanf (seedtimestr, "%d%*[,:.]%d%*[,:.]%d%*[,:.]%d%*[,:.]%d%f",
		   &year, &day, &hour, &min, &sec, &fusec);
  
  /* Convert fractional seconds to microseconds */
  if ( fusec != 0.0 )
    {
      usec = (int) (fusec * 1000000.0 + 0.5);
    }
  
  if ( fields < 1 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error converting time string: %s\n", seedtimestr);
      return HPTERROR;
    }
  
  if ( year < 1900 || year > 3000 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error with year value: %d\n", year);
      return HPTERROR;
    }

  if ( day < 1 || day > 366 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error with day value: %d\n", day);
      return HPTERROR;
    }
  
  if ( hour < 0 || hour > 23 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error with hour value: %d\n", hour);
      return HPTERROR;
    }
  
  if ( min < 0 || min > 59 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error with minute value: %d\n", min);
      return HPTERROR;
    }
  
  if ( sec < 0 || sec > 60 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error with second value: %d\n", sec);
      return HPTERROR;
    }
  
  if ( usec < 0 || usec > 999999 )
    {
      ms_log (2, "ms_seedtimestr2hptime(): Error with fractional second value: %d\n", usec);
      return HPTERROR;
    }
  
  return ms_time2hptime_int (year, day, hour, min, sec, usec);
}  /* End of ms_seedtimestr2hptime() */


/***************************************************************************
 * ms_timestr2hptime:
 * 
 * Convert a generic time string to a high precision epoch time.
 * SEED time format is "YYYY[/MM/DD HH:MM:SS.FFFF]", the delimiter can
 * be a dash [-], slash [/], colon [:], or period [.] and between the
 * date and time a 'T' or a space may be used.  The fracttional
 * seconds must begin with a period [.].
 *
 * The time string can be "short" in which case the omitted values are
 * assumed to be zero (with the exception of month and day which are
 * assumed to be 1): "YYYY/MM/DD" assumes HH, MM, SS and FFFF are 0.
 * The year is required, otherwise there wouldn't be much for a date.
 *
 * Ranges are checked for each value.
 *
 * Returns epoch time on success and HPTERROR on error.
 ***************************************************************************/
hptime_t
ms_timestr2hptime (char *timestr)
{
  int fields;
  int year = 0;
  int mon  = 1;
  int mday = 1;
  int day  = 1;
  int hour = 0;
  int min  = 0;
  int sec  = 0;
  float fusec = 0.0;
  int usec = 0;
    
  fields = sscanf (timestr, "%d%*[-/:.]%d%*[-/:.]%d%*[-/:.T ]%d%*[-/:.]%d%*[- /:.]%d%f",
		   &year, &mon, &mday, &hour, &min, &sec, &fusec);
  
  /* Convert fractional seconds to microseconds */
  if ( fusec != 0.0 )
    {
      usec = (int) (fusec * 1000000.0 + 0.5);
    }

  if ( fields < 1 )
    {
      ms_log (2, "ms_timestr2hptime(): Error converting time string: %s\n", timestr);
      return HPTERROR;
    }
  
  if ( year < 1900 || year > 3000 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with year value: %d\n", year);
      return HPTERROR;
    }
  
  if ( mon < 1 || mon > 12 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with month value: %d\n", mon);
      return HPTERROR;
    }

  if ( mday < 1 || mday > 31 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with day value: %d\n", mday);
      return HPTERROR;
    }

  /* Convert month and day-of-month to day-of-year */
  if ( ms_md2doy (year, mon, mday, &day) )
    {
      return HPTERROR;
    }
  
  if ( hour < 0 || hour > 23 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with hour value: %d\n", hour);
      return HPTERROR;
    }
  
  if ( min < 0 || min > 59 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with minute value: %d\n", min);
      return HPTERROR;
    }
  
  if ( sec < 0 || sec > 60 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with second value: %d\n", sec);
      return HPTERROR;
    }
  
  if ( usec < 0 || usec > 999999 )
    {
      ms_log (2, "ms_timestr2hptime(): Error with fractional second value: %d\n", usec);
      return HPTERROR;
    }
  
  return ms_time2hptime_int (year, day, hour, min, sec, usec);
}  /* End of ms_timestr2hptime() */


/***************************************************************************
 * ms_genfactmult:
 *
 * Generate an approriate SEED sample rate factor and multiplier from
 * a double precision sample rate.
 * 
 * Returns 0 on success and -1 on error.
 ***************************************************************************/
int
ms_genfactmult (double samprate, int16_t *factor, int16_t *multiplier)
{
  int num, den;
  
  /* This routine does not support very high or negative sample rates,
     even though high rates are possible in Mini-SEED */
  if ( samprate > 32727.0 || samprate < 0.0 )
    {
      ms_log (2, "ms_genfactmult(): samprate out of range: %g\n", samprate);
      return -1;
    }
  
  /* If the sample rate is integer set the factor and multipler in the
     obvious way, otherwise derive a (potentially approximate)
     numerator and denominator for the given samprate */
  if ( (samprate - (int16_t) samprate) < 0.000001 )
    {
      *factor = (int16_t) samprate;
      if ( *factor )
	*multiplier = 1;
    }
  else
    {
      ms_ratapprox (samprate, &num, &den, 32727, 1e-12);
      
      /* Negate the multiplier to denote a division factor */
      *factor = (int16_t ) num;
      *multiplier = (int16_t) -den;
    }
  
  return 0;
}  /* End of ms_genfactmult() */


/***************************************************************************
 * ms_ratapprox:
 *
 * Find an approximate rational number for a real through continued
 * fraction expansion.  Given a double precsion 'real' find a
 * numerator (num) and denominator (den) whose absolute values are not
 * larger than 'maxval' while trying to reach a specified 'precision'.
 * 
 * Returns the number of iterations performed.
 ***************************************************************************/
int
ms_ratapprox (double real, int *num, int *den, int maxval, double precision)
{
  double realj, preal;
  char pos;  
  int pnum, pden;
  int iterations = 1;
  int Aj1, Aj2, Bj1, Bj2;
  int bj = 0;
  int Aj = 0;
  int Bj = 1;
  
  if ( real >= 0.0 ) { pos = 1; realj = real; }
  else               { pos = 0; realj = -real; }
  
  preal = realj;
  
  bj = (int) (realj + precision);
  realj = 1 / (realj - bj);
  Aj = bj; Aj1 = 1;
  Bj = 1;  Bj1 = 0;
  *num = pnum = Aj;
  *den = pden = Bj;
  if ( !pos ) *num = -*num;
  
  while ( ms_dabs(preal - (double)Aj/(double)Bj) > precision &&
	  Aj < maxval && Bj < maxval )
    {
      Aj2 = Aj1; Aj1 = Aj;
      Bj2 = Bj1; Bj1 = Bj;
      bj = (int) (realj + precision);
      realj = 1 / (realj - bj);
      Aj = bj * Aj1 + Aj2;
      Bj = bj * Bj1 + Bj2;
      *num = pnum;
      *den = pden;
      if ( !pos ) *num = -*num;
      pnum = Aj;
      pden = Bj;
      
      iterations++;
    }
  
  if ( pnum < maxval && pden < maxval )
    {
      *num = pnum;
      *den = pden;
      if ( !pos ) *num = -*num;
    }
  
  return iterations;
}


/***************************************************************************
 * ms_bigendianhost:
 *
 * Determine the byte order of the host machine.  Due to the lack of
 * portable defines to determine host byte order this run-time test is
 * provided.  The code below actually tests for little-endianess, the
 * only other alternative is assumed to be big endian.
 * 
 * Returns 0 if the host is little endian, otherwise 1.
 ***************************************************************************/
int
ms_bigendianhost ()
{
  int16_t host = 1;
  return !(*((int8_t *)(&host)));
}  /* End of ms_bigendianhost() */


/***************************************************************************
 * ms_dabs:
 *
 * Determine the absolute value of an input double, actually just test
 * if the input double is positive multiplying by -1.0 if not and
 * return it.
 * 
 * Returns the positive value of input double.
 ***************************************************************************/
double
ms_dabs (double val)
{
  if ( val < 0.0 )
    val *= -1.0;
  return val;
}  /* End of ms_dabs() */
