#include <stdio.h>
/* #include <malloc.h> */
#include <xview/xview.h>
#include <xview/panel.h>
#include "menudefs.h"
#include "defines.h"

/*
ParseMenus(): Read in the menu config file, and generate the internal
menu structures used by the window system.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/

extern Gmenu menu[];
int num_menus;

void ParseMenu()
{
	int j,curmenu = -1,curitem = 0;
	int curchoice = 0 ,curarg = 0,curinput = 0, curoutput = 0;
	char Inline[GBUFSIZ],temp[GBUFSIZ],head[GBUFSIZ];
	char tail[GBUFSIZ],*home;
	Gmenu *thismenu;
	GmenuItem *thisitem;
	GmenuItemArg *thisarg;
	GfileFormat *thisinput,*thisoutput;
	FILE *file;
	char *resize;

/*
*	Open the menu configuration file "GDEmenus"
*	First search the local directory, then the home directory.
*/
	file=fopen("GDEmenus","r");
	if(file == NULL)
	{
		home = (char*)getenv("HOME");
		strcpy(temp,home);
		strcat(temp,"/GDEmenus");

		file=fopen(temp,"r");
		if(file == NULL)
		{
#ifdef USE_ARB
			sprintf(temp,"%s/GDEHELP/GDEmenus",getenv("ARBHOME"));
			file=fopen(temp,"r");
			if(file == NULL)
			    Error("GDEmenus file not in the home, local, or $ARBHOME/GDEHELP directory");
#else
			home = (char*)getenv("GDE_HELP_DIR");
			if(home != NULL)
			{
				strcpy(temp,home);
				strcat(temp,"/GDEmenus");
				file=fopen(temp,"r");
			}
			if(file == NULL)
			    Error("GDEmenus file not in the home, local, or $GDE_HELP_DIR directory");
#endif
		}


	}

/*
*	Read the GDEmenus file, and assemble an internal representation
*	of the menu/menu-item hierarchy.
*/

	for(;getline(file,Inline) != EOF;)
	{
/*
*	menu: chooses menu to use
*/
		if(Inline[0] == '#');
		else if(Find(Inline,"menu:"))
		{
			crop(Inline,head,temp);
			curmenu = -1;
			for(j=0;j<num_menus;j++)
				if(Find(temp,menu[j].label))
					curmenu=j;
/*
*	If menu not found, make a new one
*/
			if(curmenu == -1)
			{
				curmenu = num_menus++;
				thismenu = &menu[curmenu];
				thismenu->label =
				(char*)calloc(strlen(temp)+1,sizeof(char));

			       	if(thismenu->label == NULL)
					Error("Calloc");
				(void)strcpy(thismenu->label,temp);
			       	thismenu->numitems = 0;
			}
		}
/*
*	item: chooses menu item to use
*/
		else if(Find(Inline,"item:"))
		{
			curarg = -1;
			curinput = -1;
			curoutput = -1;
			crop(Inline,head,temp);
			curitem = thismenu->numitems++;
/*
*	Resize the item list for this menu (add one item);
*/
			if(curitem == 0)
				resize = (char*)calloc(1,sizeof(GmenuItem));
			else
				resize = realloc(thismenu->item,
				thismenu -> numitems*sizeof(GmenuItem) );

			if(resize == NULL)
				Error ("Calloc");
			thismenu->item =(GmenuItem*)resize;

			thisitem = &(thismenu->item[curitem]);
			thisitem->label = (char*)calloc(strlen(temp)+1,
			sizeof(char));
			thisitem->meta = '\0';
			thisitem->numinputs = 0;
			thisitem->numoutputs = 0;
			thisitem->numargs = 0;
			thisitem->X = 0;
			thisitem->help = NULL;

/*
*	Create new item
*/

			if(thisitem->label == NULL)
				Error("Calloc");
			(void)strcpy(thisitem->label,temp);
		}

/*
*	itemmethod: generic command line generated by this item
*/
		else if(Find(Inline,"itemmethod:"))
		{
			crop(Inline,head,temp);
			thisitem->method =
			(char*)calloc(strlen(temp)+1,sizeof(char));
			if(thisitem->method == NULL)
				Error("Calloc");
			(void)strcpy(thisitem->method,temp);
		}
/*
*	Help file
*/
		else if(Find(Inline,"itemhelp:"))
		{
			crop(Inline,head,temp);
			thisitem->help =
			(char*)calloc(strlen(temp)+1,sizeof(char));
			if(thisitem->method == NULL)
				Error("Calloc");
			(void)strcpy(thisitem->help,temp);
		}
/*
*		Meta key equiv
*/
		else if(Find(Inline,"itemmeta:"))
		{
			crop(Inline,head,temp);
			thisitem->meta = temp[0];
		}
/*
*	arg: defines the symbol for a command line arguement.
*		this is used for substitution into the itemmethod
*		definition.
*/

		else if(Find(Inline,"arg:"))
		{
			crop(Inline,head,temp);
			curarg=thisitem->numargs++;
			if(curarg == 0)
				resize = (char*)calloc(1,sizeof(GmenuItemArg));
			else
				resize = realloc(thisitem->arg,
				thisitem->numargs*sizeof(GmenuItemArg) );


			if(resize == NULL)
				Error("arg: Realloc");

			(thisitem->arg) = (GmenuItemArg*)resize;
			thisarg = &(thisitem->arg[curarg]);
			thisarg->symbol = (char*)calloc(strlen(temp)+1,
			sizeof(char));
			if(thisarg->symbol == NULL)
				Error("Calloc");
			(void)strcpy(thisarg->symbol,temp);
			thisarg->optional = FALSE;
			thisarg->type = 0;
			thisarg->min = 0;
			thisarg->max = 0;
			thisarg->numchoices = 0;
			thisarg->choice = NULL;
			thisarg->textvalue = NULL;
			thisarg->value = 0;
		}
/*
*	argtype: Defines the type of argument (menu,chooser, text, slider)
*/
		else if(Find(Inline,"argtype:"))
		{
			crop(Inline,head,temp);
			if(strcmp(temp,"text")==0)
			{
				thisarg->type=TEXTFIELD;
				thisarg->textvalue =
					(char*)calloc(GBUFSIZ,sizeof(char));
				if(thisarg->textvalue == NULL)
					Error("Calloc");
			}
			else if(strcmp(temp,"choice_list") == 0)
				thisarg->type                   = CHOICE_LIST;
			else if(strcmp(temp,"choice_menu") == 0)
				thisarg->type                   = CHOICE_MENU;
			else if(strcmp(temp,"chooser")     == 0)
				thisarg->type                   = CHOOSER;
			else if(strcmp(temp,"slider")      == 0)
				thisarg->type                   = SLIDER;
			else {
                sprintf(head,"Unknown argtype %s",temp);
				Error(head);
            }
		}
/*
*	argtext: The default text value of the symbol.
*		$argument is replaced by this value if it is not
*		changed in the dialog box by the user.
*/
		else if(Find(Inline,"argtext:"))
		{
			crop(Inline,head,temp);
			(void)strcpy(thisarg->textvalue,temp);
		}
/*
*	arglabel: Text label displayed in the dialog box for
*		this argument.  It should be a discriptive label.
*/
		else if(Find(Inline,"arglabel:"))
		{
			crop(Inline,head,temp);
			thisarg->label=(char*)calloc(strlen(temp)+1,
			sizeof(char));
			if(thisarg->label == NULL)
				Error("Calloc");
			(void)strcpy(thisarg->label,temp);
		}
/*
*	Argument choice values use the following notation:
*
*	argchoice:Displayed value:Method
*
*	Where "Displayed value" is the label displayed in the dialog box
*	and "Method" is the value passed back on the command line.
*/
		else if(Find(Inline,"argchoice:"))
		{
			crop(Inline,head,temp);
			crop(temp,head,tail);
			curchoice = thisarg->numchoices++;
			if(curchoice == 0)
				resize = (char*)calloc(1,sizeof(GargChoice));
			else
				resize = realloc(thisarg->choice,
				thisarg->numchoices*sizeof(GargChoice));

			if(resize == NULL)
				Error("argchoice: Realloc");
			thisarg->choice = (GargChoice*)resize;

			(thisarg->choice[curchoice].label) = NULL;
			(thisarg->choice[curchoice].method) = NULL;

			(thisarg->choice[curchoice].label) =
			(char*)calloc(strlen(head)+1,sizeof(char));

			(thisarg->choice[curchoice].method) =
			(char*)calloc(strlen(tail)+1,sizeof(char));

			if(thisarg->choice[curchoice].method == NULL ||
			thisarg->choice[curchoice].label == NULL)
				Error("Calloc");

			(void)strcpy(thisarg->choice[curchoice].label,head);
			(void)strcpy(thisarg->choice[curchoice].method,tail);
		}
/*
*	argmin: Minimum value for a slider
*/
		else if(Find(Inline,"argmin:"))
		{
			crop(Inline,head,temp);
			(void)sscanf(temp,"%d",&(thisarg->min));
		}
/*
*	argmax: Maximum value for a slider
*/
		else if(Find(Inline,"argmax:"))
		{
			crop(Inline,head,temp);
			(void)sscanf(temp,"%d",&(thisarg->max));
		}
/*
*	argmethod: Command line flag associated with this argument.
*		Replaces argument in itemmethod description.
*/
		else if(Find(Inline,"argmethod:"))
		{
			crop(Inline,head,temp);
			thisarg->method = (char*)calloc(GBUFSIZ,strlen(temp));
				if(thisarg->method == NULL)
					Error("Calloc");
			(void)strcpy(thisarg->method,tail);
		}
/*
*	argvalue: default value for a slider
*/
		else if(Find(Inline,"argvalue:"))
		{
			crop(Inline,head,temp);
			if(thisarg->type == TEXT)
				strcpy(thisarg->textvalue,temp);
			else
				(void)sscanf(temp,"%d",&(thisarg->value));
		}
/*
*	argoptional: Flag specifying that an arguement is optional
*/
		else if(Find(Inline,"argoptional:"))
			thisarg->optional = TRUE;
/*
*	in: Input file description
*/
		else if(Find(Inline,"in:"))
		{
			crop(Inline,head,temp);
			curinput = (thisitem->numinputs)++;
			if(curinput == 0)
				resize = (char*)calloc(1,sizeof(GfileFormat));
			else
				resize = realloc(thisitem->input,
				(thisitem->numinputs)*sizeof(GfileFormat));

			if(resize == NULL)
				Error("in: Realloc");
			thisitem->input = (GfileFormat*)resize;
			thisinput = &(thisitem->input)[curinput];
			thisinput->save = FALSE;
			thisinput->overwrite = FALSE;
			thisinput->maskable = FALSE;
			thisinput->format = 0;
			thisinput->symbol = String(temp);
			thisinput->name = NULL;
			thisinput->select = SELECTED;
		}

/*
*	out: Output file description
*/

		else if(Find(Inline,"out:"))
		{
			crop(Inline,head,temp);
			curoutput = (thisitem->numoutputs)++;
			if(curoutput == 0)
				resize = (char*)calloc(1,sizeof(GfileFormat));
			else
				resize = realloc(thisitem->output,
				(thisitem->numoutputs)*sizeof(GfileFormat));

			if(resize == NULL)
				Error("out: Realloc");
			thisitem->output = (GfileFormat*)resize;
			thisoutput = &(thisitem->output)[curoutput];
			thisitem->output = (GfileFormat*)resize;
			thisoutput = &(thisitem->output)[curoutput];
			thisoutput->save = FALSE;
			thisoutput->overwrite = FALSE;
			thisoutput->format = 0;
			thisoutput->symbol= String(temp);
			thisoutput->name = NULL;
		}
		else if(Find(Inline,"informat:"))
		{
			if(thisinput == NULL)
				Error("Problem with GDEmenus");
			crop(Inline,head,tail);
			if(Find(tail,"genbank"))
				thisinput->format = GENBANK;
			else if(Find(tail,"gde"))
				thisinput->format = GDE;
			else if(Find(tail,"na_flat"))
				thisinput->format = NA_FLAT;
			else if(Find(tail,"colormask"))
				thisinput->format = COLORMASK;
			else if(Find(tail,"flat"))
				thisinput->format = NA_FLAT;
			else if(Find(tail,"status"))
				thisinput->format = STATUS_FILE;
			else fprintf(stderr,"Warning, unknown file format %s\n"
					,tail);
		}
		else if(Find(Inline,"insave:"))
		{
			if(thisinput == NULL)
				Error("Problem with GDEmenus");
			thisinput->save = TRUE;
		}
		else if(Find(Inline,"inselect:"))
		{
			if(thisinput == NULL)
				Error("Problem with GDEmenus");
						crop(Inline,head,tail);
						if(Find(tail,"one"))
							thisinput->select = SELECT_ONE;
						else if(Find(tail,"region"))
							thisinput->select = SELECT_REGION;
						else if(Find(tail,"all"))
							thisinput->select = ALL;
		}
		else if(Find(Inline,"inmask:"))
		{
			if(thisinput == NULL)
				Error("Problem with GDEmenus");
			thisinput->maskable = TRUE;
		}
		else if(Find(Inline,"outformat:"))
		{
			if(thisoutput == NULL)
				Error("Problem with GDEmenus");
			crop(Inline,head,tail);
			if(Find(tail,"genbank"))
				thisoutput->format = GENBANK;
			else if(Find(tail,"gde"))
				thisoutput->format = GDE;
			else if(Find(tail,"na_flat"))
				thisoutput->format = NA_FLAT;
			else if(Find(tail,"flat"))
				thisoutput->format = NA_FLAT;
			else if(Find(tail,"status"))
				thisoutput->format = STATUS_FILE;
			else if(Find(tail,"colormask"))
				thisoutput->format = COLORMASK;
			else fprintf(stderr,"Warning, unknown file format %s\n"
					,tail);
		}
		else if(Find(Inline,"outsave:"))
		{
			if(thisoutput == NULL)
				Error("Problem with GDEmenus");
			thisoutput->save = TRUE;
		}
		else if(Find(Inline,"outoverwrite:"))
		{
			if(thisoutput == NULL)
				Error("Problem with GDEmenus");
			thisoutput->overwrite = TRUE;
		}
	}
	return;
}



/*
Find(): Search the target string for the given key
*/
int Find(target,key)
char *target;
const char *key;
{
	int i,j,len1,dif,flag = FALSE;
	dif = (strlen(target)) - (len1 = strlen(key)) +1;

	if(len1>0)
		for(j=0;j<dif && flag == FALSE;j++)
		{
			flag = TRUE;
			for(i=0;i<len1 && flag;i++)
				flag = (key[i] == target[i+j])?TRUE:FALSE;

		}
	return(flag);
}


int Find2(target,key)
     char       *target;
     const char *key;
     /*
      *	Like find, but returns the index of the leftmost
      *	occurence, and -1 if not found.
      */
{
	int i,j,len1,dif,flag = FALSE;
	dif = (strlen(target)) - (len1 = strlen(key)) +1;

	if(len1>0)
		for(j=0;j<dif && flag == FALSE;j++)
		{
			flag = TRUE;
			for(i=0;i<len1 && flag;i++)
				flag = (key[i] == target[i+j])?TRUE:FALSE;

		}
	return(flag?j-1:-1);
}


void Error(msg)
     const char *msg;
{
	(void)fprintf(stderr,"%s\n",msg);
	exit(1);
}


int getline(file,string)
FILE *file;
char string[];
{
	char c;
	int i;
	for(i=0;((c=getc(file))!='\n') && (c!=EOF);i++)
		string[i]=c;
	string[i] = '\0';
	if (i==0 && c==EOF) return (EOF);
	else return (0);
}

/*
Crop():
	Split "this:that[:the_other]"
	into: "this" and "that[:the_other]"
*/

void crop(input,head,tail)
char input[],head[],tail[];
{
/*
*	Crop needs to be fixed so that whitespace is compressed off the end
*	of tail
*/
	int offset,end,i,j,length;

	length=strlen(input);
	for(offset=0;offset<length && input[offset] != ':';offset++)
		head[offset]=input[offset];
	head[offset++] = '\0';
	for(;offset<length && input[offset] == ' ';offset++);
	for(end=length-1;input[end] ==' ' && end>offset;end--);

	for(j=0,i=offset;i<=end;i++,j++)
		tail[j]=input[i];
	tail[j] = '\0';
	return;
}
