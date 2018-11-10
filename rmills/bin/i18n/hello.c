// Tutorial http://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html
//
// Build the code:
// $ make -B hello LDFLAGS=-lintl

// Generate pot (template)
// $ mkdir -p  po
// $ xgettext --keyword=_ --language=C --add-comments --sort-output -o po/hello.pot hello.c

// Generate french po (translation)
// $ mkdir -p po/fr
// $ msginit --input=po/hello.pot --locale=fr --output=po/fr/hello.po

// Edit po/fr/hello.po "Hello World" => "Bonjour le monde"

// Generate mo        (messages)
// $ mkdir -p fr/LC_MESSAGES/
// $ msgfmt --output-file=fr/LC_MESSAGES/hello.mo po/fr/hello.po

// Run
// $ env LC_ALL=fr_FR ./hello
//
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)

int main()
{
    // Setting the i18n environment
    setlocale (LC_ALL, "");
    bindtextdomain ("hello", getenv("PWD"));
    textdomain ("hello");

    // Example of i18n usage
    printf("%s\n",_("Hello World"));

    return EXIT_SUCCESS;
}
