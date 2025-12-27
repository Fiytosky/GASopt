#include "datascope.h"
#include "symbols.h"
#include "subsegs.h"

#include <limits.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

segT xom_section;

symbolS *text_cur_symbol;
const char* cur_func_name;
char *log_file;

bool is_in_func;
bool is_finish_subseg;
unsigned long long text_frag_index;
meteheaderS mete_header;

void xom_section_init () {
	xom_section = subseg_new (XOM_SECTION_NAME, 0);
	bfd_set_section_flags (xom_section, SEC_HAS_CONTENTS);

	subseg_set (xom_section, 0);
	as_warn (_("Initialize xom section"));
}

static void
log_file_init () {
	char *filename = (char*)malloc(64 * sizeof(char));
	if (filename == NULL) {
		as_fatal ("Failed to create log file -> memory allocation failed");
		return;
	}
	sprintf(filename, "/root/xom/test/tmp/data_%d.txt", getpid());

	log_file = filename;
	as_warn (_("The log will be written in file %s"), log_file);
}

void datascope_init () {
	text_cur_symbol = NULL;
	cur_func_name = NULL;

	is_in_func = false;
	is_finish_subseg = false;
	text_frag_index = 0;

	log_file_init ();
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

// metedata
void write_meteheader () {
	gas_assert (now_seg == xom_section);

	strcpy (mete_header.magic, "xom");
	unsigned int x;
	const char *filename;
	filename = as_where (&x);
	if (!filename) {
		filename = "unkown_name";
	}
	unsigned int size = strlen (filename) + 1;
	mete_header.name_size = size;

	size_t total_size = sizeof(meteheaderS) + size;
	char *p = frag_more (total_size);

	memcpy (p, &mete_header, sizeof(meteheaderS));
	memcpy (p + sizeof(meteheaderS), filename, size);
	as_warn (_("meteheader filename: %s, write_size: %u"), filename, total_size);
}

void write_metedata (metedataS *data, const char *syn_name) {
	gas_assert (now_seg == xom_section);
	
	size_t total_size = sizeof(metedataS) + data->symbol_size;
	char *p = frag_more (total_size);

	memcpy (p, data, sizeof(metedataS));
	memcpy (p + sizeof(metedataS), syn_name, data->symbol_size);
	as_warn (_("write frag_info, frag_index: %u, write_size: %u"), data->frag_index, total_size);
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


