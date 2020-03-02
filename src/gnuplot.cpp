/** @file gnuplot.cpp
 *
 * File created by Peter Watkins (KE7IST) 1/8/18.
 * Derived from original project code.
 * Splat!
 * @copyright 1997 - 2018 John A. Magliacane (KD2BD) and contributors.
 * See revision control history for contributions.
 * This file is covered by the LICENSE.md file in the root of this project.
 */

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <bzlib.h>
#include "site.h"
#include "path.h"
#include "dem.h"
#include "pat_file.h"
#include "splat_run.h"
#include "lrp.h"
#include "sdf.h"
#include "elevation_map.h"
#include "gnuplot.h"
#include "splat_run.h"
#include "utilities.h"

using namespace std;

void GnuPlot::GraphTerrain(const Site &source, const Site &destination, const string &name, const ElevationMap &em)
{
    /* This function invokes gnuplot to generate an appropriate
     output file indicating the terrain profile between the source
     and destination locations when the -p command line option
     is used.  "basename" is the name assigned to the output
     file generated by gnuplot.  The filename extension is used
     to set gnuplot's terminal setting and output file type.
     If no extension is found, .png is assumed.  */
    
    int	x, z;
    char	basename[255], term[30], ext[15];
    double	minheight=100000.0, maxheight=-100000.0;
    FILE	*fd=NULL, *fd1=NULL;
    
    path.ReadPath(destination,source,em);
    
    fd=fopen("profile.gp","wb");
    
    if (sr.clutter>0.0)
        fd1=fopen("clutter.gp","wb");
    
    for (x=0; x<path.length; x++)
    {
        if ((path.elevation[x]+sr.clutter)>maxheight)
            maxheight=path.elevation[x]+sr.clutter;
        
        if (path.elevation[x]<minheight)
            minheight=path.elevation[x];
        
        if (sr.metric)
        {
            fprintf(fd,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*path.elevation[x]);
            
            if (fd1!=NULL && x>0 && x<path.length-2)
                fprintf(fd1,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*(path.elevation[x]==0.0?path.elevation[x]:(path.elevation[x]+sr.clutter)));
        }
        
        else
        {
            fprintf(fd,"%f\t%f\n",path.distance[x],path.elevation[x]);
            
            if (fd1!=NULL && x>0 && x<path.length-2)
                fprintf(fd1,"%f\t%f\n",path.distance[x],(path.elevation[x]==0.0?path.elevation[x]:(path.elevation[x]+sr.clutter)));
        }
    }
    
    fclose(fd);
    
    if (fd1!=NULL)
        fclose(fd1);
    
    if (name[0]=='.')
    {
        /* Default filename and output file type */
        
        strncpy(basename,"profile\0",8);
        strncpy(term,"png\0",4);
        strncpy(ext,"png\0",4);
    }
    
    else
    {
        /* Extract extension and terminal type from "name" */
        
        ext[0]=0;
        size_t y = strlen(name.c_str());
        strncpy(basename,name.c_str(),254);
        
        for (x=(int) y - 1; x>0 && name[x]!='.'; x--);
        
        if (x>0)  /* Extension found */
        {
            for (z=x+1; z<=y && (z-(x+1))<10; z++)
            {
                ext[z-(x+1)]=tolower(name[z]);
                term[z-(x+1)]=name[z];
            }
            
            ext[z-(x+1)]=0;  /* Ensure an ending 0 */
            term[z-(x+1)]=0;
            basename[x]=0;
        }
        
        if (ext[0]==0)	/* No extension -- Default is png */
        {
            strncpy(term,"png\0",4);
            strncpy(ext,"png\0",4);
        }
    }
    
    /* Either .ps or .postscript may be used
     as an extension for postscript output. */
    
    if (strncmp(term,"postscript",10)==0)
        strncpy(ext,"ps\0",3);
    
    else if (strncmp(ext,"ps",2)==0)
        strncpy(term,"postscript enhanced color\0",26);
    
    if (maxheight<1.0)
    {
        maxheight=1.0;	/* Avoid a gnuplot y-range error */
        minheight=-1.0;	/* over a completely sea-level path */
    }
    
    else
        minheight-=(0.01*maxheight);
    
    fd=fopen("splat.gp","w");
    fprintf(fd,"set grid\n");
    fprintf(fd,"set yrange [%2.3f to %2.3f]\n", sr.metric?minheight*METERS_PER_FOOT:minheight, sr.metric?maxheight*METERS_PER_FOOT:maxheight);
    fprintf(fd,"set encoding iso_8859_1\n");
    fprintf(fd,"set term %s\n",term);
    fprintf(fd,"set title \"%s Terrain Profile Between %s and %s (%.2f%c Azimuth)\"\n",SplatRun::splat_name.c_str(),destination.name.c_str(), source.name.c_str(), destination.Azimuth(source),176);
    
    if (sr.metric)
    {
        fprintf(fd,"set xlabel \"Distance Between %s and %s (%.2f kilometers)\"\n",destination.name.c_str(),source.name.c_str(),KM_PER_MILE*source.Distance(destination));
        fprintf(fd,"set ylabel \"Ground Elevation Above Sea Level (meters)\"\n");
    }
    
    else
    {
        fprintf(fd,"set xlabel \"Distance Between %s and %s (%.2f miles)\"\n",destination.name.c_str(),source.name.c_str(),source.Distance(destination));
        fprintf(fd,"set ylabel \"Ground Elevation Above Sea Level (feet)\"\n");
    }
    
    fprintf(fd,"set output \"%s.%s\"\n",basename,ext);
    
    if (sr.clutter>0.0)
    {
        if (sr.metric)
            fprintf(fd,"plot \"profile.gp\" title \"Terrain Profile\" with lines, \"clutter.gp\" title \"sr.clutter Profile (%.2f meters)\" with lines\n",sr.clutter*METERS_PER_FOOT);
        else
            fprintf(fd,"plot \"profile.gp\" title \"Terrain Profile\" with lines, \"clutter.gp\" title \"sr.clutter Profile (%.2f feet)\" with lines\n",sr.clutter);
    }
    
    else
        fprintf(fd,"plot \"profile.gp\" title \"\" with lines\n");
    
    fclose(fd);
    
    x=system("gnuplot splat.gp");
    
    if (x!=-1)
    {
        if (!sr.gpsav)
        {
            unlink("splat.gp");
            unlink("profile.gp");
        }
        
        fprintf(stdout,"Terrain plot written to: \"%s.%s\"\n",basename,ext);
        fflush(stdout);
    }
    
    else
        fprintf(stderr,"\n*** ERROR: Error occurred invoking gnuplot!\n");
}

void GnuPlot::GraphElevation(const Site &source, const Site &destination, const string &name, const ElevationMap &em)
{
    /* This function invokes gnuplot to generate an appropriate
     output file indicating the terrain elevation profile between
     the source and destination locations when the -e command line
     option is used.  "basename" is the name assigned to the output
     file generated by gnuplot.  The filename extension is used
     to set gnuplot's terminal setting and output file type.
     If no extension is found, .png is assumed.  */
    
    int	x, z;
    char	basename[255], term[30], ext[15];
    double	angle, clutter_angle=0.0, refangle, maxangle=-90.0,
    minangle=90.0, distance;
    Site remote, remote2;
    FILE	*fd=NULL, *fd1=NULL, *fd2=NULL;
    
    path.ReadPath(destination,source,em);  /* destination=RX, source=TX */
    refangle=em.ElevationAngle(path, destination, source);
    distance=source.Distance(destination);
    
    fd=fopen("profile.gp","wb");
    
    if (sr.clutter>0.0)
        fd1=fopen("clutter.gp","wb");
    
    fd2=fopen("reference.gp","wb");
    
    for (x=1; x<path.length-1; x++)
    {
        remote.lat=path.lat[x];
        remote.lon=path.lon[x];
        remote.alt=0.0;
        angle=em.ElevationAngle(path, destination, remote);
        
        if (sr.clutter>0.0)
        {
            remote2.lat=path.lat[x];
            remote2.lon=path.lon[x];
            
            if (path.elevation[x]!=0.0)
                remote2.alt=sr.clutter;
            else
                remote2.alt=0.0;
            
            clutter_angle=em.ElevationAngle(path, destination, remote2);
        }
        
        if (sr.metric)
        {
            fprintf(fd,"%f\t%f\n",KM_PER_MILE*path.distance[x],angle);
            
            if (fd1!=NULL)
                fprintf(fd1,"%f\t%f\n",KM_PER_MILE*path.distance[x],clutter_angle);
            
            fprintf(fd2,"%f\t%f\n",KM_PER_MILE*path.distance[x],refangle);
        }
        
        else
        {
            fprintf(fd,"%f\t%f\n",path.distance[x],angle);
            
            if (fd1!=NULL)
                fprintf(fd1,"%f\t%f\n",path.distance[x],clutter_angle);
            
            fprintf(fd2,"%f\t%f\n",path.distance[x],refangle);
        }
        
        if (angle>maxangle)
            maxangle=angle;
        
        if (clutter_angle>maxangle)
            maxangle=clutter_angle;
        
        if (angle<minangle)
            minangle=angle;
    }
    
    if (sr.metric)
    {
        fprintf(fd,"%f\t%f\n",KM_PER_MILE*path.distance[path.length-1],refangle);
        fprintf(fd2,"%f\t%f\n",KM_PER_MILE*path.distance[path.length-1],refangle);
    }
    
    else
    {
        fprintf(fd,"%f\t%f\n",path.distance[path.length-1],refangle);
        fprintf(fd2,"%f\t%f\n",path.distance[path.length-1],refangle);
    }
    
    fclose(fd);
    
    if (fd1!=NULL)
        fclose(fd1);
    
    fclose(fd2);
    
    if (name[0]=='.')
    {
        /* Default filename and output file type */
        
        strncpy(basename,"profile\0",8);
        strncpy(term,"png\0",4);
        strncpy(ext,"png\0",4);
    }
    
    else
    {
        /* Extract extension and terminal type from "name" */
        
        ext[0]=0;
        size_t y = strlen(name.c_str());
        strncpy(basename,name.c_str(),254);
        
        for (x = (int) y - 1; x>0 && name[x]!='.'; x--);
        
        if (x>0)  /* Extension found */
        {
            for (z=x+1; z<=y && (z-(x+1))<10; z++)
            {
                ext[z-(x+1)]=tolower(name[z]);
                term[z-(x+1)]=name[z];
            }
            
            ext[z-(x+1)]=0;  /* Ensure an ending 0 */
            term[z-(x+1)]=0;
            basename[x]=0;
        }
        
        if (ext[0]==0)	/* No extension -- Default is png */
        {
            strncpy(term,"png\0",4);
            strncpy(ext,"png\0",4);
        }
    }
    
    /* Either .ps or .postscript may be used
     as an extension for postscript output. */
    
    if (strncmp(term,"postscript",10)==0)
        strncpy(ext,"ps\0",3);
    
    else if (strncmp(ext,"ps",2)==0)
        strncpy(term,"postscript enhanced color\0",26);
    
    fd=fopen("splat.gp","w");
    
    fprintf(fd,"set grid\n");
    
    if (distance>2.0)
        fprintf(fd,"set yrange [%2.3f to %2.3f]\n", (-fabs(refangle)-0.25), maxangle+0.25);
    else
        fprintf(fd,"set yrange [%2.3f to %2.3f]\n", minangle, refangle+(-minangle/8.0));
    
    fprintf(fd,"set encoding iso_8859_1\n");
    fprintf(fd,"set term %s\n",term);
    fprintf(fd,"set title \"%s Elevation Profile Between %s and %s (%.2f%c azimuth)\"\n",SplatRun::splat_name.c_str(),destination.name.c_str(),source.name.c_str(),destination.Azimuth(source),176);
    
    if (sr.metric)
        fprintf(fd,"set xlabel \"Distance Between %s and %s (%.2f kilometers)\"\n",destination.name.c_str(),source.name.c_str(),KM_PER_MILE*distance);
    else
        fprintf(fd,"set xlabel \"Distance Between %s and %s (%.2f miles)\"\n",destination.name.c_str(),source.name.c_str(),distance);
    
    
    fprintf(fd,"set ylabel \"Elevation Angle Along LOS Path Between\\n%s and %s (degrees)\"\n",destination.name.c_str(),source.name.c_str());
    fprintf(fd,"set output \"%s.%s\"\n",basename,ext);
    
    if (sr.clutter>0.0)
    {
        if (sr.metric)
            fprintf(fd,"plot \"profile.gp\" title \"Real Earth Profile\" with lines, \"clutter.gp\" title \"sr.clutter Profile (%.2f meters)\" with lines, \"reference.gp\" title \"Line of Sight Path (%.2f%c elevation)\" with lines\n",sr.clutter*METERS_PER_FOOT,refangle,176);
        else
            fprintf(fd,"plot \"profile.gp\" title \"Real Earth Profile\" with lines, \"clutter.gp\" title \"sr.clutter Profile (%.2f feet)\" with lines, \"reference.gp\" title \"Line of Sight Path (%.2f%c elevation)\" with lines\n",sr.clutter,refangle,176);
    }
    
    else
        fprintf(fd,"plot \"profile.gp\" title \"Real Earth Profile\" with lines, \"reference.gp\" title \"Line of Sight Path (%.2f%c elevation)\" with lines\n",refangle,176);
    
    fclose(fd);
    
    x=system("gnuplot splat.gp");
    
    if (x!=-1)
    {
        if (!sr.gpsav)
        {
            unlink("splat.gp");
            unlink("profile.gp");
            unlink("reference.gp");
            
            if (sr.clutter>0.0)
                unlink("clutter.gp");
        }
        
        fprintf(stdout,"Elevation plot written to: \"%s.%s\"\n",basename,ext);
        fflush(stdout);
    }
    
    else
        fprintf(stderr,"\n*** ERROR: Error occurred invoking gnuplot!\n");
}

void GnuPlot::GraphHeight(const Site &source, const Site &destination, const string &name, bool fresnel_plot, bool normalized, const ElevationMap &em, const Lrp &lrp)
{
    /* This function invokes gnuplot to generate an appropriate
     output file indicating the terrain height profile between
     the source and destination locations referenced to the
     line-of-sight path between the receive and transmit sites
     when the -h or -H command line option is used.  "basename"
     is the name assigned to the output file generated by gnuplot.
     The filename extension is used to set gnuplot's terminal
     setting and output file type.  If no extension is found,
     .png is assumed.  */
    
    int	x, z;
    char	basename[255], term[30], ext[15];
    double	a, b, c, height=0.0, refangle, cangle, maxheight=-100000.0,
    minheight=100000.0, lambda=0.0, f_zone=0.0, fpt6_zone=0.0,
    nm=0.0, nb=0.0, ed=0.0, es=0.0, r=0.0, d=0.0, d1=0.0,
    terrain, azimuth, distance, dheight=0.0, minterrain=100000.0,
    minearth=100000.0, miny, maxy, min2y, max2y;
    Site	remote;
    FILE	*fd=NULL, *fd1=NULL, *fd2=NULL, *fd3=NULL, *fd4=NULL, *fd5=NULL;
    
    path.ReadPath(destination, source, em);  /* destination=RX, source=TX */
    azimuth=destination.Azimuth(source);
    distance=destination.Distance(source);
    refangle=em.ElevationAngle(path, destination, source);
    b=em.GetElevation(destination)+destination.alt+sr.earthradius;
    
    /* Wavelength and path distance (great circle) in feet. */
    
    if (fresnel_plot)
    {
        lambda=9.8425e8/(lrp.frq_mhz*1e6);
        d=5280.0*path.distance[path.length-1];
    }
    
    if (normalized)
    {
        ed=em.GetElevation(destination);
        es=em.GetElevation(source);
        nb=-destination.alt-ed;
        nm=(-source.alt-es-nb)/(path.distance[path.length-1]);
    }
    
    fd=fopen("profile.gp","wb");
    
    if (sr.clutter>0.0)
        fd1=fopen("clutter.gp","wb");
    
    fd2=fopen("reference.gp","wb");
    fd5=fopen("curvature.gp", "wb");
    
    if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
    {
        fd3=fopen("fresnel.gp", "wb");
        fd4=fopen("fresnel_pt_6.gp", "wb");
    }
    
    for (x=0; x<path.length-1; x++)
    {
        remote.lat=path.lat[x];
        remote.lon=path.lon[x];
        remote.alt=0.0;
        
        terrain=em.GetElevation(remote);
        
        if (x==0)
            terrain+=destination.alt;  /* RX antenna spike */
        
        a=terrain+sr.earthradius;
        cangle=5280.0*destination.Distance(remote)/sr.earthradius;
        c=b*sin(refangle*DEG2RAD+HALFPI)/sin(HALFPI-refangle*DEG2RAD-cangle);
        
        height=a-c;
        
        /* Per Fink and Christiansen, Electronics
         * Engineers' Handbook, 1989:
         *
         *   H = sqrt(lamba * d1 * (d - d1)/d)
         *
         * where H is the distance from the LOS
         * path to the first Fresnel zone boundary.
         */
        
        if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
        {
            d1=5280.0*path.distance[x];
            f_zone=-1.0*sqrt(lambda*d1*(d-d1)/d);
            fpt6_zone=f_zone*sr.fzone_clearance;
        }
        
        if (normalized)
        {
            r=-(nm*path.distance[x])-nb;
            height+=r;
            
            if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
            {
                f_zone+=r;
                fpt6_zone+=r;
            }
        }
        
        else
            r=0.0;
        
        if (sr.metric)
        {
            fprintf(fd,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*height);
            
            if (fd1!=NULL && x>0 && x<path.length-2)
                fprintf(fd1,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*(terrain==0.0?height:(height+sr.clutter)));
            
            fprintf(fd2,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*r);
            fprintf(fd5,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*(height-terrain));
        }
        
        else
        {
            fprintf(fd,"%f\t%f\n",path.distance[x],height);
            
            if (fd1!=NULL && x>0 && x<path.length-2)
                fprintf(fd1,"%f\t%f\n",path.distance[x],(terrain==0.0?height:(height+sr.clutter)));
            
            fprintf(fd2,"%f\t%f\n",path.distance[x],r);
            fprintf(fd5,"%f\t%f\n",path.distance[x],height-terrain);
        }
        
        if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
        {
            if (sr.metric)
            {
                fprintf(fd3,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*f_zone);
                fprintf(fd4,"%f\t%f\n",KM_PER_MILE*path.distance[x],METERS_PER_FOOT*fpt6_zone);
            }
            
            else
            {
                fprintf(fd3,"%f\t%f\n",path.distance[x],f_zone);
                fprintf(fd4,"%f\t%f\n",path.distance[x],fpt6_zone);
            }
            
            if (f_zone<minheight)
                minheight=f_zone;
        }
        
        if ((height+sr.clutter)>maxheight)
            maxheight=height+sr.clutter;
        
        if (height<minheight)
            minheight=height;
        
        if (r>maxheight)
            maxheight=r;
        
        if (terrain<minterrain)
            minterrain=terrain;
        
        if ((height-terrain)<minearth)
            minearth=height-terrain;
    }
    
    if (normalized)
        r=-(nm*path.distance[path.length-1])-nb;
    else
        r=0.0;
    
    if (sr.metric)
    {
        fprintf(fd,"%f\t%f\n",KM_PER_MILE*path.distance[path.length-1],METERS_PER_FOOT*r);
        fprintf(fd2,"%f\t%f\n",KM_PER_MILE*path.distance[path.length-1],METERS_PER_FOOT*r);
    }
    
    else
    {
        fprintf(fd,"%f\t%f\n",path.distance[path.length-1],r);
        fprintf(fd2,"%f\t%f\n",path.distance[path.length-1],r);
    }
    
    if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
    {
        if (sr.metric)
        {
            fprintf(fd3,"%f\t%f\n",KM_PER_MILE*path.distance[path.length-1],METERS_PER_FOOT*r);
            fprintf(fd4,"%f\t%f\n",KM_PER_MILE*path.distance[path.length-1],METERS_PER_FOOT*r);
        }
        
        else
        {
            fprintf(fd3,"%f\t%f\n",path.distance[path.length-1],r);
            fprintf(fd4,"%f\t%f\n",path.distance[path.length-1],r);
        }
    }
    
    if (r>maxheight)
        maxheight=r;
    
    if (r<minheight)
        minheight=r;
    
    fclose(fd);
    
    if (fd1!=NULL)
        fclose(fd1);
    
    fclose(fd2);
    fclose(fd5);
    
    if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
    {
        fclose(fd3);
        fclose(fd4);
    }
    
    if (name[0]=='.')
    {
        /* Default filename and output file type */
        
        strncpy(basename,"profile\0",8);
        strncpy(term,"png\0",4);
        strncpy(ext,"png\0",4);
    }
    
    else
    {
        /* Extract extension and terminal type from "name" */
        
        ext[0]=0;
        size_t y = strlen(name.c_str());
        strncpy(basename,name.c_str(),254);
        
        for (x= (int) y - 1; x>0 && name[x]!='.'; x--);
        
        if (x>0)  /* Extension found */
        {
            for (z=x+1; z<=y && (z-(x+1))<10; z++)
            {
                ext[z-(x+1)]=tolower(name[z]);
                term[z-(x+1)]=name[z];
            }
            
            ext[z-(x+1)]=0;  /* Ensure an ending 0 */
            term[z-(x+1)]=0;
            basename[x]=0;
        }
        
        if (ext[0]==0)	/* No extension -- Default is png */
        {
            strncpy(term,"png\0",4);
            strncpy(ext,"png\0",4);
        }
    }
    
    /* Either .ps or .postscript may be used
     as an extension for postscript output. */
    
    if (strncmp(term,"postscript",10)==0)
        strncpy(ext,"ps\0",3);
    
    else if (strncmp(ext,"ps",2)==0)
        strncpy(term,"postscript enhanced color\0",26);
    
    fd=fopen("splat.gp","w");
    
    dheight=maxheight-minheight;
    miny=minheight-0.15*dheight;
    maxy=maxheight+0.05*dheight;
    
    if (maxy<20.0)
        maxy=20.0;
    
    dheight=maxheight-minheight;
    min2y=miny-minterrain+0.05*dheight;
    
    if (minearth<min2y)
    {
        miny-=min2y-minearth+0.05*dheight;
        min2y=minearth-0.05*dheight;
    }
    
    max2y=min2y+maxy-miny;
    
    fprintf(fd,"set grid\n");
    fprintf(fd,"set yrange [%2.3f to %2.3f]\n", sr.metric?miny*METERS_PER_FOOT:miny, sr.metric?maxy*METERS_PER_FOOT:maxy);
    fprintf(fd,"set y2range [%2.3f to %2.3f]\n", sr.metric?min2y*METERS_PER_FOOT:min2y, sr.metric?max2y*METERS_PER_FOOT:max2y);
    fprintf(fd,"set xrange [-0.5 to %2.3f]\n",sr.metric?KM_PER_MILE*rint(distance+0.5):rint(distance+0.5));
    fprintf(fd,"set encoding iso_8859_1\n");
    fprintf(fd,"set term %s\n",term);
    
    if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
        fprintf(fd,"set title \"%s Path Profile Between %s and %s (%.2f%c azimuth)\\nWith First Fresnel Zone\"\n",SplatRun::splat_name.c_str(), destination.name.c_str(), source.name.c_str(), azimuth,176);
    
    else
        fprintf(fd,"set title \"%s Height Profile Between %s and %s (%.2f%c azimuth)\"\n",SplatRun::splat_name.c_str(), destination.name.c_str(), source.name.c_str(), azimuth,176);
    
    if (sr.metric)
        fprintf(fd,"set xlabel \"Distance Between %s and %s (%.2f kilometers)\"\n",destination.name.c_str(),source.name.c_str(),KM_PER_MILE*source.Distance(destination));
    else
        fprintf(fd,"set xlabel \"Distance Between %s and %s (%.2f miles)\"\n",destination.name.c_str(),source.name.c_str(),source.Distance(destination));
    
    if (normalized)
    {
        if (sr.metric)
            fprintf(fd,"set ylabel \"Normalized Height Referenced To LOS Path Between\\n%s and %s (meters)\"\n",destination.name.c_str(),source.name.c_str());
        
        else
            fprintf(fd,"set ylabel \"Normalized Height Referenced To LOS Path Between\\n%s and %s (feet)\"\n",destination.name.c_str(),source.name.c_str());
        
    }
    
    else
    {
        if (sr.metric)
            fprintf(fd,"set ylabel \"Height Referenced To LOS Path Between\\n%s and %s (meters)\"\n",destination.name.c_str(),source.name.c_str());
        
        else
            fprintf(fd,"set ylabel \"Height Referenced To LOS Path Between\\n%s and %s (feet)\"\n",destination.name.c_str(),source.name.c_str());
    }
    
    fprintf(fd,"set output \"%s.%s\"\n",basename,ext);
    
    if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
    {
        if (sr.clutter>0.0)
        {
            if (sr.metric)
                fprintf(fd,"plot \"profile.gp\" title \"Point-to-Point Profile\" with lines, \"clutter.gp\" title \"Ground sr.clutter (%.2f meters)\" with lines, \"reference.gp\" title \"Line of Sight Path\" with lines, \"curvature.gp\" axes x1y2 title \"Earth's Curvature Contour\" with lines, \"fresnel.gp\" axes x1y1 title \"First Fresnel Zone (%.3f MHz)\" with lines, \"fresnel_pt_6.gp\" title \"%.0f%% of First Fresnel Zone\" with lines\n",sr.clutter*METERS_PER_FOOT,lrp.frq_mhz,sr.fzone_clearance*100.0);
            else
                fprintf(fd,"plot \"profile.gp\" title \"Point-to-Point Profile\" with lines, \"clutter.gp\" title \"Ground sr.clutter (%.2f feet)\" with lines, \"reference.gp\" title \"Line of Sight Path\" with lines, \"curvature.gp\" axes x1y2 title \"Earth's Curvature Contour\" with lines, \"fresnel.gp\" axes x1y1 title \"First Fresnel Zone (%.3f MHz)\" with lines, \"fresnel_pt_6.gp\" title \"%.0f%% of First Fresnel Zone\" with lines\n",sr.clutter,lrp.frq_mhz,sr.fzone_clearance*100.0);
        }
        
        else
            fprintf(fd,"plot \"profile.gp\" title \"Point-to-Point Profile\" with lines, \"reference.gp\" title \"Line of Sight Path\" with lines, \"curvature.gp\" axes x1y2 title \"Earth's Curvature Contour\" with lines, \"fresnel.gp\" axes x1y1 title \"First Fresnel Zone (%.3f MHz)\" with lines, \"fresnel_pt_6.gp\" title \"%.0f%% of First Fresnel Zone\" with lines\n",lrp.frq_mhz,sr.fzone_clearance*100.0);
    }
    
    else
    {
        if (sr.clutter>0.0)
        {
            if (sr.metric)
                fprintf(fd,"plot \"profile.gp\" title \"Point-to-Point Profile\" with lines, \"clutter.gp\" title \"Ground sr.clutter (%.2f meters)\" with lines, \"reference.gp\" title \"Line Of Sight Path\" with lines, \"curvature.gp\" axes x1y2 title \"Earth's Curvature Contour\" with lines\n",sr.clutter*METERS_PER_FOOT);
            else
                fprintf(fd,"plot \"profile.gp\" title \"Point-to-Point Profile\" with lines, \"clutter.gp\" title \"Ground sr.clutter (%.2f feet)\" with lines, \"reference.gp\" title \"Line Of Sight Path\" with lines, \"curvature.gp\" axes x1y2 title \"Earth's Curvature Contour\" with lines\n",sr.clutter);
        }
        
        else
            fprintf(fd,"plot \"profile.gp\" title \"Point-to-Point Profile\" with lines, \"reference.gp\" title \"Line Of Sight Path\" with lines, \"curvature.gp\" axes x1y2 title \"Earth's Curvature Contour\" with lines\n");
        
    }
    
    fclose(fd);
    
    x=system("gnuplot splat.gp");
    
    if (x!=-1)
    {
        if (!sr.gpsav)
        {
            unlink("splat.gp");
            unlink("profile.gp");
            unlink("reference.gp");
            unlink("curvature.gp");
            
            if (fd1!=NULL)
                unlink("clutter.gp");
            
            if ((lrp.frq_mhz>=20.0) && (lrp.frq_mhz<=20000.0) && fresnel_plot)
            {
                unlink("fresnel.gp");
                unlink("fresnel_pt_6.gp");
            }
        }
        
        fprintf(stdout,"\nHeight plot written to: \"%s.%s\"",basename,ext);
        fflush(stdout);
    }
    
    else
        fprintf(stderr,"\n*** ERROR: Error occurred invoking gnuplot!\n");
}
