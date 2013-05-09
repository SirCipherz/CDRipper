/******************************************************
*
*     ©keithhedger Sun  5 May 19:03:12 BST 2013
*     kdhedger68713@gmail.com
*
*     disc.cpp
* 
******************************************************/

#include <cddb/cddb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <glib.h>
#include <linux/cdrom.h>
#include <cddb/cddb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "globals.h"

GtkWidget*	progressWindow;
char		ripName[1024];
GtkWidget*	label;
bool		labelChanged=false;

cddb_disc_t* readDisc(void)
{
	int fd;
	int status;
	int i;
	char trackname[9];
	struct cdrom_tochdr th;
	struct cdrom_tocentry te;

	cddb_disc_t* disc=NULL;
	cddb_track_t* track=NULL;

    // open the device
	fd=open(cdrom,O_RDONLY|O_NONBLOCK);
	if(fd<0)
		{
			fprintf(stderr,"Error: Couldn't open %s\n",cdrom);
			return NULL;
		}

	// read disc status info
	status=ioctl(fd,CDROM_DISC_STATUS,CDSL_CURRENT);
	if ((status==CDS_AUDIO) || (status==CDS_MIXED))
		{
			// see if we can read the disc's table of contents (TOC).
			if (ioctl(fd, CDROMREADTOCHDR, &th)==0)
				{
					startTrack=th.cdth_trk0;
					numTracks=th.cdth_trk1;

					disc=cddb_disc_new();
					if (disc==NULL)
						printf("cddb_disc_new() failed. Out of memory?");

					te.cdte_format=CDROM_LBA;
					for (i=th.cdth_trk0;i<=th.cdth_trk1;i++)
						{
							te.cdte_track=i;
							if(ioctl(fd,CDROMREADTOCENTRY,&te)==0)
								{
									track=cddb_track_new();
									if (track==NULL)
										printf("cddb_track_new() failed. Out of memory?");

									cddb_track_set_frame_offset(track,te.cdte_addr.lba+SECONDS_TO_FRAMES(2));
									snprintf(trackname,9,"Track %d",i);
									cddb_track_set_title(track,trackname);
									cddb_track_set_artist(track,"Unknown Artist");
									cddb_disc_add_track(disc,track);
								}
						}

					te.cdte_track=CDROM_LEADOUT;
					if (ioctl(fd,CDROMREADTOCENTRY,&te)==0)
						cddb_disc_set_length(disc,(te.cdte_addr.lba+SECONDS_TO_FRAMES(2))/SECONDS_TO_FRAMES(1));
				}
		}

	close(fd);

	return(disc);
}

GList* lookupDisc(cddb_disc_t* disc)
{
	GList*			matches=NULL;
	cddb_conn_t*	connection;
	int				numMatches;

	// set up the connection to the cddb server
	connection=cddb_new();
	if (connection==NULL)
		printf("cddb_new() failed. Out of memory?");
    
//	cddb_set_server_name(connection,"freedb.freedb.org");
	cddb_set_server_name(connection,"freedb.musicbrainz.org");
	cddb_set_server_port(connection,8880);

	numMatches=cddb_query(connection, disc);

	// make a list of all the matches
	for (int i=0;i<numMatches;i++)
		{
			cddb_disc_t* possible_match=cddb_disc_clone(disc);
			if (!cddb_read(connection,possible_match))
				{
					cddb_error_print(cddb_errno(connection));
					printf("cddb_read() failed.");
				}
			matches=g_list_append(matches,possible_match);
        
		// move to next match
		if (i<numMatches-1)
			{
				if(!cddb_query_next(connection,disc))
					printf("Query index out of bounds.");
			}
		}

	cddb_destroy(connection);
    
    return matches;
}

void printDetails(cddb_disc_t* disc)
{
	char*			disc_artist=(char*)cddb_disc_get_artist(disc);
	char*			disc_title=(char*)cddb_disc_get_title(disc);
	char*			disc_genre=(char*)cddb_disc_get_genre(disc);
	unsigned		disc_year=cddb_disc_get_year(disc);
	cddb_track_t*	track;
	int				tracknum=1;

	if(disc_artist!=NULL)
		{
			printf("Artist - %s\n",disc_artist);
			artist=(char*)cddb_disc_get_artist(disc);
		}
	if(disc_artist!=NULL)
		{
			printf("Album - %s\n",disc_title);
			album=(char*)cddb_disc_get_title(disc);
		}

	printf("Genre - %s\n",disc_genre);
	printf("Year - %i\n",disc_year);

	for (track=cddb_disc_get_track_first(disc); track != NULL; track=cddb_disc_get_track_next(disc))
        {
			printf("Track %2.2i - %s\n",tracknum,cddb_track_get_title(track));
			printf("Track Artist - %s\n",cddb_track_get_artist(track));
  			tracknum++;
        }
}
//cdda2wav dev=/dev/cdrom -t ${TNUM}+${TNUM} -alltracks -max
/*
	asprintf(&command,"curl -sk \"https://ajax.googleapis.com/ajax/services/search/images?v=1.0&q=%s+%s&as_filetype=jpg&imgsz=large&rsz=1\"",artist,album);

	fp=popen(command, "r");
	fgets((char*)&buffer[0],16384,fp);

	url=sliceBetween((char*)buffer,(char*)"unescapedUrl\":\"",(char*)"\",\"");
	if(url!=NULL)
		{
			printf("%s\n",url);
			if(download==true)
				{
					asprintf(&command,"curl -sko folder.jpg %s",url);
					system(command);
				}
		}
	pclose(fp);
	if(command!=NULL)
		g_free(command);
//curl -sk "http://musicbrainz.org/search?query=various+story+songs&type=release&method=indexed" > curlout
//curl -sk "http://musicbrainz.org/release/5b3432b9-0f01-447b-8dbd-9a7f4f1bf61e/cover-art"
//<img src="http://ecx.images-amazon.com/images/I/51LlZiD3uFL.jpg" />
//	asprintf(&command,"curl -sk \"https://ajax.googleapis.com/ajax/services/search/images?v=1.0&q=%s+%s&as_filetype=jpg&imgsz=large&rsz=1\"",artist,album);

*/

void getAlbumArt()
{
	char*			command;
	FILE*			fp=NULL;
	GString*		buffer=g_string_new(NULL);;
	char			line[1024];
	char*			urlmb=NULL;
	char*			release=NULL;
	char*			artwork;
	char*			flacimage;
	char*			mp4image;
	char*			mp3image;
	const char*		artistfolder;

	asprintf(&album,"%s",gtk_entry_get_text((GtkEntry*)albumEntry));
	asprintf(&artist,"%s",gtk_entry_get_text((GtkEntry*)artistEntry));

	if(isCompilation==true)
		artistfolder=COMPILATIONARTIST;
	else
		artistfolder=artist;

	asprintf(&flacimage,"%s/%s/%s/folder.jpg",flacFolder,artistfolder,album);
	asprintf(&mp4image,"%s/%s/%s/folder.jpg",mp4Folder,artistfolder,album);
	asprintf(&mp3image,"%s/%s/%s/folder.jpg",mp3Folder,artistfolder,album);

	album=g_strdelimit(album," ",'+');
	artist=g_strdelimit(artist," ",'+');

	asprintf(&command,"curl -sk \"http://musicbrainz.org/search?query=%s+%s&type=release&method=indexed\"",artist,album);
	fp=popen(command, "r");
	g_free(command);
	if(fp!=NULL)
		{
			while(fgets(line,1024,fp))
				g_string_append_printf(buffer,"%s",line);
			pclose(fp);
		}

	urlmb=strstr(buffer->str,"href=\"http://musicbrainz.org/release");
	if(urlmb!=NULL)
		{
			release=sliceBetween(urlmb,(char*)"href=\"",(char*)"\">");
			if(release!=NULL)
				{
					asprintf(&command,"curl -sk \"%s/cover-art\"",release);
					fp=popen(command, "r");
					g_free(command);

					g_string_erase(buffer,0,-1);
					while(fgets(line,1024,fp))
						g_string_append_printf(buffer,"%s",line);
					pclose(fp);

					artwork=sliceBetween(buffer->str,(char*)"<img src=\"",(char*)"\"");
					if(artwork!=NULL)
						{
							asprintf(&command,"wget \"%s\" -O \"%s\"",artwork,flacimage);
							system(command);
							g_free(command);
							asprintf(&command,"cp \"%s\" \"%s\"",flacimage,mp4image);
							system(command);
							g_free(command);
							asprintf(&command,"cp \"%s\" \"%s\"",flacimage,mp3image);
							system(command);
							g_free(command);
						}
				}
			}

	if(release!=NULL)
		g_free(release);
	if(artwork!=NULL)
		g_free(artwork);
	if(flacimage!=NULL)
		g_free(flacimage);
	if(mp4image!=NULL)
		g_free(mp4image);
	if(mp3image!=NULL)
		g_free(mp3image);

	g_string_free(buffer,true);
}

gboolean doneRipping(gpointer data)
{
    GtkWidget *dialog;

    g_source_remove(GPOINTER_TO_INT(g_object_get_data((GObject*)data, "source_id")));
    gtk_widget_destroy(GTK_WIDGET(data));

    return false;
}

gboolean updateBarTimer(gpointer data)
{
	if(GTK_IS_PROGRESS_BAR((GtkProgressBar*)data))
		{
			if(labelChanged==true)
				{
					labelChanged=false;
					gtk_label_set_text((GtkLabel*)label,ripName);
				}
			gtk_progress_bar_pulse((GtkProgressBar*)data);
			return(true);
		}
	else
		return(false);
}

gpointer doTheRip(gpointer data)
{
	int				tracknum=1;
	char*			command;
	FILE*			fp;
	cddb_disc_t*	disc=(cddb_disc_t*)data;
	char*			filename=NULL;
	char*			cdstr=g_strdup(gtk_entry_get_text((GtkEntry*)cdEntry));
	char*			cdnum=NULL;
	char*			tagdata;
	const char*		artistfolder;

	if(isCompilation==true)
		artistfolder=COMPILATIONARTIST;
	else
		artistfolder=gtk_entry_get_text((GtkEntry*)artistEntry);

	asprintf(&command,"%s/%s/%s",flacFolder,artistfolder,gtk_entry_get_text((GtkEntry*)albumEntry));
	g_mkdir_with_parents(command,493);
	g_free(command);
	asprintf(&command,"%s/%s/%s",mp4Folder,artistfolder,gtk_entry_get_text((GtkEntry*)albumEntry));
	g_mkdir_with_parents(command,493);
	g_free(command);
	asprintf(&command,"%s/%s/%s",mp3Folder,artistfolder,gtk_entry_get_text((GtkEntry*)albumEntry));
	g_mkdir_with_parents(command,493);
	g_free(command);

	if(strchr(cdstr,'/')!=NULL)
		asprintf(&cdnum,"%i-",atoi(cdstr));
	else
		asprintf(&cdnum,"%s","");

	for (int i=1;i<=numTracks;i++)
		{
			g_chdir(tmpDir);
			if(gtk_toggle_button_get_active((GtkToggleButton*)ripThis[i])==true)
				{
					sprintf((char*)&ripName,"Ripping Track \"%s\" ...",gtk_entry_get_text((GtkEntry*)trackName[i]));
					labelChanged=true;
					asprintf(&command,"cdda2wav dev=/dev/cdrom -t %i+%i -alltracks -max",tracknum,tracknum);
					system(command);
					g_free(command);

//set tags
					asprintf(&tagdata,"kute --artist=\"%s\" --album=\"%s\" --title=\"%s\" --track=%i --total-tracks=%i --year=\"%s\" --genre=\"%s\" --comment=\"Ripped and tagged by K.D.Hedger\" --cd=\"%s\""
					,gtk_entry_get_text((GtkEntry*)trackArtist[i])
					,gtk_entry_get_text((GtkEntry*)albumEntry)
					,gtk_entry_get_text((GtkEntry*)trackName[i])
					,i
					,numTracks
					,gtk_entry_get_text((GtkEntry*)yearEntry)
					,gtk_entry_get_text((GtkEntry*)genreEntry)
					,cdstr
					);

					filename=sliceDeleteRange((char*)gtk_entry_get_text((GtkEntry*)trackName[i])," :/'&^%$!{}@;?.");

					if(ripFlac==true)
						{
							system("flac -f --fast audio.wav");
							asprintf(&command,"%s audio.flac",tagdata);
							system(command);
							g_free(command);
							asprintf(&command,"mv audio.flac \"%s/%s/%s/%s%2.2i %s.flac\"",flacFolder,artistfolder,gtk_entry_get_text((GtkEntry*)albumEntry),cdnum,i,filename);
							system(command);
							g_free(command);
						}
					if(ripMp4==true)
						{
							system("ffmpeg -i audio.wav -q:a 0 audio.m4a");
							asprintf(&command,"%s audio.m4a",tagdata);
							system(command);
							g_free(command);
							asprintf(&command,"mv audio.m4a \"%s/%s/%s/%s%2.2i %s.m4a\"",mp4Folder,artistfolder,gtk_entry_get_text((GtkEntry*)albumEntry),cdnum,i,filename);
							system(command);
							g_free(command);
						}
					if((ripMp3==true))
						{
							system("ffmpeg -i audio.wav -q:a 0 audio.mp3");
							asprintf(&command,"%s audio.mp3",tagdata);
							system(command);
							g_free(command);
							asprintf(&command,"mv audio.mp3 \"%s/%s/%s/%s%2.2i %s.mp3\"",mp3Folder,artistfolder,gtk_entry_get_text((GtkEntry*)albumEntry),cdnum,i,filename);
							system(command);
							g_free(command);
						}

					g_free(tagdata);
					g_free(filename);

					gtk_toggle_button_set_active((GtkToggleButton*)ripThis[i],false);
				}
		}

	getAlbumArt();
	g_idle_add(doneRipping,progressWindow);
	g_thread_exit(NULL);
}

void ripTracks(GtkWidget* widg,gpointer data)
{	
	GtkWidget*	widget;
	GtkWidget*	vbox;
    guint		sid;

	/* create the modal window which warns the user to wait */
	progressWindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(progressWindow),true);
	gtk_window_set_title(GTK_WINDOW(progressWindow),"Ripping, Please Wait...");
	gtk_container_set_border_width(GTK_CONTAINER(progressWindow), 12);
	gtk_window_set_transient_for((GtkWindow*)progressWindow,window);
	gtk_window_set_default_size((GtkWindow*)progressWindow,400,-1);
	g_signal_connect(progressWindow,"delete_event",G_CALLBACK(gtk_true), NULL);
	vbox=gtk_vbox_new(false,12);
	gtk_widget_show(vbox);
	/* create label */
	label=gtk_label_new("Ripping, Please wait...");
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(vbox),label);
	/* create progress bar */
	widget=gtk_progress_bar_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(vbox),widget);
	/* add vbox to dialog */
	gtk_container_add(GTK_CONTAINER(progressWindow),vbox);
	gtk_widget_show(progressWindow);
	sid=g_timeout_add(100, updateBarTimer,widget);
	g_object_set_data(G_OBJECT(progressWindow), "source_id",GINT_TO_POINTER(sid));

#if GLIB_MINOR_VERSION < PREFERVERSION
	g_thread_create(doTheRip,(void*)data,false,NULL);
#else
	g_thread_new("redo",(GThreadFunc)doTheRip,data);
#endif
}

void doShutdown(GtkWidget* widget,gpointer data)
{
	justQuit=(bool)data;
	gtk_main_quit();
}

void doNothing(GtkWidget* widget,gpointer data)
{
	printf("Use A Button\n");
}


void doSensitive(GtkWidget* widget,gpointer data)
{
	bool	sens=gtk_toggle_button_get_active((GtkToggleButton*)ripThis[(long)data]);

	gtk_widget_set_sensitive(trackName[(long)data],sens);
	gtk_widget_set_sensitive(trackArtist[(long)data],sens);
}

void doCompiliation(GtkWidget* widget,gpointer data)
{
	bool	sens=gtk_toggle_button_get_active((GtkToggleButton*)widget);

	gtk_widget_set_sensitive(artistEntry,!sens);
	isCompilation=sens;

	if(sens==true)
		{
			gtk_entry_set_text((GtkEntry*)artistEntry,COMPILATIONSTRING);
		}
	else
		{
			gtk_entry_set_text((GtkEntry*)artistEntry,artist);
		}
}

void doSelectAll(GtkWidget* widget,gpointer data)
{
	bool	sens=gtk_toggle_button_get_active((GtkToggleButton*)widget);

	for(int i=1;i<=numTracks;i++)
		gtk_toggle_button_set_active((GtkToggleButton*)ripThis[i],sens);
}

void doDetails(cddb_disc_t* disc)
{
	char*			disc_artist=(char*)cddb_disc_get_artist(disc);
	char*			disc_title=(char*)cddb_disc_get_title(disc);
	char*			disc_genre=(char*)cddb_disc_get_genre(disc);
	unsigned		disc_year=cddb_disc_get_year(disc);
	cddb_track_t*	track;
	int				tracknum=1;
	char*			tmpstr;

	GtkWidget*		hbox;

	GtkWidget*		compilation;

	if(detailsVBox!=NULL)
		gtk_widget_destroy(detailsVBox);

	detailsVBox=gtk_vbox_new(false,0);
	hbox=gtk_hbox_new(false,0);

//disc artist
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Artist:		"),false,false,0);
	artistEntry=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),artistEntry,true,true,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);

	if(disc_artist!=NULL)
		{
			artist=(char*)cddb_disc_get_artist(disc);
			gtk_entry_set_text((GtkEntry*)artistEntry,artist);
		}

//disc title
	hbox=gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Album:		"),false,false,0);
	albumEntry=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),albumEntry,true,true,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);

	if(disc_title!=NULL)
		{
			album=(char*)cddb_disc_get_title(disc);
			gtk_entry_set_text((GtkEntry*)albumEntry,album);
		}

//genre
	hbox=gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Genre:		"),false,false,0);
	genreEntry=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),genreEntry,true,true,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);

	if(disc_genre!=NULL)
		{
			genre=disc_genre;
			gtk_entry_set_text((GtkEntry*)genreEntry,disc_genre);
		}
			
//year
	hbox=gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Year:		"),false,false,0);
	yearEntry=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),yearEntry,true,true,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);

	if(disc_year!=0)
		{
			asprintf(&tmpstr,"%i",disc_year);
			gtk_entry_set_text((GtkEntry*)yearEntry,tmpstr);
			year=disc_year;
			g_free(tmpstr);
		}

//cd number
	hbox=gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("CD Number:	"),false,false,0);
	cdEntry=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),cdEntry,true,true,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);
	gtk_entry_set_text((GtkEntry*)cdEntry,"1");

//comp
	hbox=gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Compilation:	"),false,false,0);
	compilation=gtk_check_button_new_with_label("");
	gtk_box_pack_start(GTK_BOX(hbox),compilation,true,true,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);
	gtk_entry_set_text((GtkEntry*)cdEntry,"1");
	g_signal_connect(G_OBJECT(compilation),"toggled",G_CALLBACK(doCompiliation),NULL);

	gtk_box_pack_start(GTK_BOX(detailsVBox),gtk_hseparator_new(),false,true,4);

//rip all
	hbox=gtk_hbox_new(false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Rip All:"),false,false,0);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new(""),true,false,0);
	ripThis[0]=gtk_check_button_new_with_label("");
	g_signal_connect(G_OBJECT(ripThis[0]),"toggled",G_CALLBACK(doSelectAll),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),ripThis[0],false,false,0);
	gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);

	gtk_box_pack_start(GTK_BOX(detailsVBox),gtk_hseparator_new(),false,true,4);

	for (track=cddb_disc_get_track_first(disc); track != NULL; track=cddb_disc_get_track_next(disc))
		{
			hbox=gtk_hbox_new(false,0);
			gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new("Track Artist:	"),false,false,0);
			trackArtist[tracknum]=gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(hbox),trackArtist[tracknum],true,true,0);
			gtk_entry_set_text((GtkEntry*)trackArtist[tracknum],cddb_track_get_artist(track));

			if(strcasecmp(cddb_track_get_artist(track),artist)!=0)
				{
					gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);
				}

		//	printf("Track Artist - %s\n",cddb_track_get_artist(track));
			hbox=gtk_hbox_new(false,0);
			asprintf(&tmpstr,"Track %2.2i:		",tracknum);
			gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new(tmpstr),false,false,0);
			g_free(tmpstr);
			trackName[tracknum]=gtk_entry_new();
			gtk_box_pack_start(GTK_BOX(hbox),trackName[tracknum],true,true,0);
			gtk_entry_set_text((GtkEntry*)trackName[tracknum],cddb_track_get_title(track));

			gtk_widget_set_sensitive(trackName[tracknum],startSelect);
			gtk_widget_set_sensitive(trackArtist[tracknum],startSelect);

			ripThis[tracknum]=gtk_check_button_new_with_label("");
			gtk_box_pack_start(GTK_BOX(hbox),ripThis[tracknum],false,false,0);
			g_signal_connect(G_OBJECT(ripThis[tracknum]),"toggled",G_CALLBACK(doSensitive),(void*)(long)tracknum);

			gtk_box_pack_start(GTK_BOX(detailsVBox),hbox,false,true,0);
		//	printf("Track %2.2i - %s\n",tracknum,cddb_track_get_title(track));
  			tracknum++;
		}

	gtk_widget_show_all(detailsVBox);
	gtk_scrolled_window_add_with_viewport((GtkScrolledWindow*)windowScrollbox,detailsVBox);
}

void freeData(gpointer data)
{
	cddb_disc_destroy((cddb_disc_t*)data);
}

void reScanCD(GtkWidget* widget,gpointer data)
{
	cddb_disc_t*	thedisc;
	cddb_disc_t*	tempdisc;

	numTracks=0;
	isCompilation=false;

	thedisc=readDisc();
	if(thedisc==NULL)
		{
			printf("no disc\n");
			return;
		}

	g_list_free_full(discMatches,freeData);
	discMatches=NULL;

	discMatches=lookupDisc(thedisc);
	if (discMatches==NULL)
		{
			printf("No matches found for disc :(\n");
			return;
		}
	cddb_disc_destroy(thedisc);

	tempdisc=(cddb_disc_t *)discMatches->data;

	doDetails(tempdisc);
}

void doRipOptions(GtkWidget* widget,gpointer data)
{
	int		what=(int)(long)data;
	bool	value=gtk_toggle_button_get_active((GtkToggleButton*)widget);

	switch (what)
		{
			case RIPFLAC:
				ripFlac=value;
				break;
			case RIPMP4:
				ripMp4=value;
				break;
			case RIPMP3:
				ripMp3=value;
				break;
		}
}

void showCDDetails(cddb_disc_t* disc)
{
	GtkWidget*		button;
	GtkWidget*		hbox;

	window=(GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(window,APPNAME);
	gtk_window_set_default_size((GtkWindow*)window,600,480);
	g_signal_connect(G_OBJECT(window),"delete-event",G_CALLBACK(doNothing),NULL);

	mainWindowVBox=gtk_vbox_new(false,0);

	gtk_container_add(GTK_CONTAINER(window),mainWindowVBox);

	gtk_widget_show_all((GtkWidget*)window);

	windowScrollbox=gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(windowScrollbox),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(mainWindowVBox),windowScrollbox);

	doDetails(disc);

	gtk_box_pack_start(GTK_BOX(mainWindowVBox),gtk_hseparator_new(),false,true,4);

	hbox=gtk_hbox_new(true,0);

//rip
	button=gtk_button_new_with_label("Rip CD");
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(ripTracks),(void*)disc);

//rip options
//rip flac
	button=gtk_check_button_new_with_label("Rip Flac");
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_toggle_button_set_active((GtkToggleButton*)button,true);
	g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(doRipOptions),(void*)RIPFLAC);
//rip mp4
	button=gtk_check_button_new_with_label("Rip Mp4");
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(doRipOptions),(void*)RIPMP4);
//rip mp3
	button=gtk_check_button_new_with_label("Rip Mp3");
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(doRipOptions),(void*)RIPMP3);

//rescan
	button=gtk_button_new_with_label("Re-Scan CD");
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(reScanCD),(void*)disc);

//quit
	button=gtk_button_new_from_stock(GTK_STOCK_QUIT);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(doShutdown),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_box_pack_start(GTK_BOX(mainWindowVBox),hbox,false,true,0);

	gtk_box_pack_start(GTK_BOX(mainWindowVBox),gtk_hseparator_new(),false,true,4);

	gtk_widget_show_all((GtkWidget*)window);
	gtk_main();
}









