#include "datascope.h"
#include "symbols.h"
#include "frags.h"

#include <limits.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

symbolS *text_cur_symbol;
char *log_file;

void datascope_init () {
	text_cur_symbol = NULL;

	char *filename = (char*)malloc(64 * sizeof(char));
	if (filename == NULL) {
		as_fatal ("Failed to create log file -> memory allocation failed");
		return;
	}
	srand(time(0));
  unsigned rand_num = rand();
	snprintf(filename, 64, "/root/xom/test/tmp/data_%x.log", rand_num);

	log_file = filename;
	as_warn (_("The log will be written in file %s"), log_file);
}

void datascope_clean () {
	free (log_file);
}

void update_cur_symbol (symbolS *sym) {
	text_cur_symbol = sym;
}

void update_frag_symbol (void) {
	frag_now->frag_symbol = text_cur_symbol;
}

// log
// copy from messages.c
static void
identify (const char *file)
{
  static int identified;

  if (identified)
    return;
  identified++;

  if (!file)
    {
      unsigned int x;
      file = as_where (&x);
    }

  if (file)
    fprintf (stderr, "%s: ", file);
  fprintf (stderr, _("Assembler messages:\n"));
}

static void
as_datascope_internal (const char *file, unsigned int line, char *buffer)
{
  bool context = false;

  if (file == NULL)
    {
      file = as_where_top (&line);
      context = true;
    }

	FILE *fp = fopen(log_file, "a");
	if (fp) {
		identify (file);
		if (file)
			{
				if (line != 0)
		fprintf (fp, "%s:%u: %s%s\n", file, line, _("Warning: "), buffer);
				else
		fprintf (fp, "%s: %s%s\n", file, _("Warning: "), buffer);
			}
		else
			fprintf (fp, "%s%s\n", _("Warning: "), buffer);

		fclose(fp);
	} else {
		identify (file);
		if (file)
			{
				if (line != 0)
		fprintf (stderr, "%s:%u: %s%s\n", file, line, _("Warning: "), buffer);
				else
		fprintf (stderr, "%s: %s%s\n", file, _("Warning: "), buffer);
			}
		else
			fprintf (stderr, "%s%s\n", _("Warning: "), buffer);
	}

  if (context)
    as_report_context ();
}

void as_datascope (const char *format, ...) {
	va_list args;
  char buffer[2000];

  if (!flag_no_warnings) {
		va_start (args, format);
		vsnprintf (buffer, sizeof (buffer), format, args);
		va_end (args);
		as_datascope_internal ((char *) NULL, 0, buffer);
	}
}


