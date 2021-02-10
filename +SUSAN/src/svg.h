#ifndef SVG_H
#define SVG_H

#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <string>
#include "datatypes.h"

class SvgCtf {
	
public:
	SvgCtf(const char*filename,const float in_apix) {
		fp = fopen(filename,"w");
		apix = in_apix;
		fprintf(fp,"<svg height=\"530\" width=\"800\" style=\"fill:white;stroke:black;stroke-width:2;\" font-family=\"Arial,Helvetica,sans-serif\">\n");
		has_avg = false;
		has_est = false;
		has_fit = false;
	}
	
	~SvgCtf() {
		fprintf(fp,"</svg>\n");
		fclose(fp);
	}
	
	void create_grid(float res_min, float res_max, float N) {
		float y_val[] = {1,0.75,0.5,0.143,0};
		float x,y;

        /// Background and Grid
        fprintf(fp,"  <rect x=\"60\" y=\"40\" width=\"720\" height=\"400\" style=\"stroke-width:0\"/>\n");
        fprintf(fp,"  <g style=\"fill:" SUSAN_SVG_SHADOW_BG ";stroke-width:0\">\n");
        x = 720*(2*res_min/N);
        fprintf(fp,"  <rect x=\"60\" y=\"40\" width=\"%.1f\" height=\"400\"/>\n",x);
        x = 720*(2*res_max/N);
        fprintf(fp,"  <rect x=\"%.1f\" y=\"40\" width=\"%.1f\" height=\"400\"/>\n",x+60,720-x);
        fprintf(fp,"  </g>\n");
        fprintf(fp,"  <g style=\"stroke:#E6E6E6\">\n");
        for(int i=1;i<10;i++) {
            x = 72.0*i + 60;
            fprintf(fp,"    <line x1=\"%.1f\" y1=\"40\" x2=\"%.1f\" y2=\"440\"/>\n",x,x);
        }
        for(int i=1;i<5;i++) {
            y =(1-y_val[i])*400 + 40;
            fprintf(fp,"    <line x1=\"60\" y1=\"%.0f\" x2=\"780\" y2=\"%.0f\"/>\n",y,y);
        }
        fprintf(fp,"  </g>\n");
        fprintf(fp,"  <rect x=\"60\" y=\"40\" width=\"720\" height=\"400\" style=\"fill:none\"/>\n");
        
        /// XY label:
        fprintf(fp,"  <text x=\"20\" y=\"240\" dominant-baseline=\"middle\" text-anchor=\"middle\" transform=\"rotate(-90 20 240)\" style=\"fill:black;stroke:none;font-size:18px;\">Normalized Amplitude</text>\n");
        fprintf(fp,"  <text x=\"420\" y=\"482\" dominant-baseline=\"middle\" text-anchor=\"middle\" style=\"fill:black;stroke:none;font-size:18px;\">Resolution (Å)</text>\n");

		/// Y ticks:
		fprintf(fp,"  <g>\n");
        for(int i=0;i<5;i++) {
            y =(1-y_val[i])*400 + 40;
            fprintf(fp,"    <text x=\"55\" y=\"%.2f\" dominant-baseline=\"middle\" text-anchor=\"end\" style=\"fill:black;stroke:none;font-size:12px;\">%.2f</text>\n",y,y_val[i]);
        }
        fprintf(fp,"  </g>\n");

		/// X ticks:
        fprintf(fp,"  <g>\n");
        for(int i=1;i<=10;i++) {
            float x_apix = 20*apix/i;
            x = 72*i + 60;
            fprintf(fp,"    <text x=\"%.2f\" y=\"445\" dominant-baseline=\"middle\" text-anchor=\"end\" transform=\"rotate(-45 %.2f 445)\" style=\"fill:black;stroke:none;font-size:12px;\">%.2f</text>\n",x,x,x_apix);
        }
        fprintf(fp,"  </g>\n");
	}
	
	void create_title(const int n_proj,const float def) {
        fprintf(fp,"  <text x=\"400\" y=\"20\" dominant-baseline=\"middle\" text-anchor=\"middle\" style=\"fill:black;stroke:none;font-weight:bold;font-size:20px\">Average Defocus for projection %d: %.2fÅ</text>\n",n_proj,def);
    }
	
	void add_avg(const float*ptr,const float M) {
		add_signal(ptr,M,SUSAN_SVG_FG_A);
		has_avg = true;
	}
	
	void add_fit(const float*ptr,const float M) {
		add_signal(ptr,M,SUSAN_SVG_FG_B);
		has_fit = true;
	}
	
	void add_est(const float*ptr,const float M) {
		fprintf(fp,"  <g style=\"stroke:" SUSAN_SVG_FG_C ";fill:none\">\n");
		fprintf(fp,"    <polyline points=\"60,40");
		float x,y,prev_x=60,prev_y=40;
		for(int i=0;i<M;i++) {
			if( ptr[i] > 0 ) {
				x = i;
				x = 60 + 720*(x/(M-1));
				y = (1-ptr[i])*400 + 40;
				fprintf(fp," %.2f,%.2f",(prev_x+x)/2,prev_y);
				fprintf(fp," %.2f,%.2f",(prev_x+x)/2,y);
				prev_x = x;
				prev_y = y;
			}
        }
        fprintf(fp," 780,%.2f",y);
        fprintf(fp,"\" />\n");
        fprintf(fp,"  </g>\n");
		has_est = true;
	}
	
	void create_legend() {
		fprintf(fp,"  <g>\n");
        fprintf(fp,"    <rect x=\"60\" y=\"495\" width=\"720\" height=\"25\"/>\n");
        if( has_avg ) {
			create_legend_entry("Radial Average",SUSAN_SVG_FG_A,60+15);
		}
		if( has_fit ) {
			create_legend_entry("Estimated CTF",SUSAN_SVG_FG_B,60+15+220);
		}
		if( has_est ) {
			create_legend_entry("Phase matching coefficient",SUSAN_SVG_FG_C,60+15+220+220);
		}
        fprintf(fp,"  </g>\n");
	}
	
	void create_legend_entry(const char*entry,const char*color,const int offset) {
		fprintf(fp,"    <line x1=\"%d\" y1=\"507.5\" x2=\"%d\" y2=\"507.5\" style=\"stroke:%s\"/>\n",offset,offset+40,color);
		fprintf(fp,"    <text x=\"%d\" y= \"507.5\" dominant-baseline=\"middle\" text-anchor=\"start\" style=\"fill:black;stroke:none;font-size:16px;\">%s</text>\n",offset+50,entry);
	}
	
protected:
	FILE*fp;
	float apix;
	
	bool has_avg;
	bool has_fit;
	bool has_est;
	
	void add_signal(const float*ptr,const float M,const char*color) {
		fprintf(fp,"  <g style=\"stroke:%s;fill:none\">\n",color);
        fprintf(fp,"    <polyline points=\"");
        for(int i=0;i<M;i++) {
            if(i>0)
                fprintf(fp," ");
            float x = i;
            x = 60 + 720*(x/(M-1));
            float y = (1-ptr[i])*400 + 40;
            fprintf(fp,"%.2f,%.2f",x,y);
        }
        fprintf(fp,"\" />\n");
        fprintf(fp,"  </g>\n");
	}
	
};

#endif 
