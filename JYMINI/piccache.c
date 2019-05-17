 
// ��ȡidx/grp����ͼ�ļ���
// Ϊ����ٶȣ����û��淽ʽ��ȡ����idx/grp�����ڴ棬Ȼ�������ɸ��������
// �������ʵ�pic���ڻ��������

#include <stdlib.h>
#include "jymain.h"

static struct PicFileCache pic_file[PIC_FILE_NUM];     

LIST_HEAD(cache_head);             //����cache����ͷ

static int currentCacheNum=0;             // ��ǰʹ�õ�cache��

static Uint32 m_color32[256];    // 256��ɫ��

extern int g_MAXCacheNum;                   // ���Cache����


extern SDL_Window* g_window;
extern SDL_Renderer* g_renderer;
extern SDL_Texture* g_screenTex;

extern int g_ScreenW ;
extern int g_ScreenH ;

extern int g_PreLoadPicGrp;
extern float g_Zoom;

int g_EnableRLE=0;

int CacheFailNum=0;

// ��ʼ��Cache���ݡ���Ϸ��ʼʱ����
int Init_Cache()
{
    int i;
    for(i=0;i<PIC_FILE_NUM;i++){
        pic_file[i].num =0;
        pic_file[i].idx =NULL;
        pic_file[i].grp=NULL;
        pic_file[i].fp=NULL;
        pic_file[i].pcache=NULL;
    }
    return 0;
}

// ��ʼ����ͼcache��Ϣ
// PalletteFilename Ϊ256��ɫ���ļ�����һ�ε���ʱ����
//                  Ϊ���ַ������ʾ���������ͼcache��Ϣ��������ͼ/����/ս���л�ʱ����
int JY_PicInit(char *PalletteFilename)
{

	struct list_head *pos,*p;
	int i;

	LoadPallette(PalletteFilename);   //�����ɫ��

	//�������Ϊ�գ�ɾ��ȫ������
	list_for_each_safe(pos,p,&cache_head){
		struct CacheNode *tmp= list_entry(pos, struct CacheNode , list);
		if(tmp->s!=NULL)
			SDL_DestroyTexture(tmp->s);       //ɾ������
		list_del(pos);
		SafeFree(tmp);
	}

	for(i=0;i<PIC_FILE_NUM;i++){
		pic_file[i].num =0;
		SafeFree(pic_file[i].idx);
		SafeFree(pic_file[i].grp);
		SafeFree(pic_file[i].pcache);
		if(pic_file[i].fp){
			fclose(pic_file[i].fp);
			pic_file[i].fp=NULL;
		}
	}

	currentCacheNum=0;
	CacheFailNum=0;
	return 0;

}

// �����ļ���Ϣ
// filename �ļ��� 
// id  0 - PIC_FILE_NUM-1
int JY_PicLoadFile(const char* idxfilename, const char* grpfilename, int id, int percent,int bufflen)
{
	int i;
	struct CacheNode *tmpcache;
	FILE *fp;

	if(id<0 || id>=PIC_FILE_NUM){  // id������Χ
		return 1;
	}

	if(pic_file[id].pcache){        //�ͷŵ�ǰ�ļ�ռ�õĿռ䣬������cache
		int i;
		for(i=0;i<pic_file[id].num;i++){   //ѭ��ȫ����ͼ��
			tmpcache=pic_file[id].pcache[i];
			if(tmpcache){       // ����ͼ�л�����ɾ��
				if(tmpcache->s!=NULL)
					SDL_DestroyTexture(tmpcache->s);       //ɾ������
				list_del(&tmpcache->list);              //ɾ������
				SafeFree(tmpcache);                  //�ͷ�cache�ڴ�
				currentCacheNum--;
			}
		}
		SafeFree(pic_file[id].pcache);
	}
	SafeFree(pic_file[id].idx);
	SafeFree(pic_file[id].grp);
	if(pic_file[id].fp){
		fclose(pic_file[id].fp);
		pic_file[id].fp=NULL;
	}


	// ��ȡidx�ļ�
	pic_file[id].bufflen = bufflen;
	pic_file[id].num =FileLength(idxfilename)/4;    //idx ��ͼ����
	pic_file[id].idx =(int *)malloc((pic_file[id].num+1)*4);
	if(pic_file[id].idx ==NULL){
		JY_Error("JY_PicLoadFile: cannot malloc idx memory!\n");
		return 1;
	}
		//��ȡ��ͼidx�ļ�
	if((fp=fopen(idxfilename,"rb"))==NULL){
		JY_Error("JY_PicLoadFile: idx file not open ---%s",idxfilename);
		return 1;
	}

	fread(&pic_file[id].idx[1],4,pic_file[id].num,fp);
	fclose(fp);

	pic_file[id].idx[0]=0;

	//��ȡgrp�ļ�
	pic_file[id].filelength=FileLength(grpfilename);

		//��ȡ��ͼgrp�ļ�
	if((fp=fopen(grpfilename,"rb"))==NULL){
		JY_Error("JY_PicLoadFile: grp file not open ---%s",grpfilename);
		return 1;
	}
	if(g_PreLoadPicGrp==1){   //grp�ļ������ڴ�
		pic_file[id].grp =(unsigned char*)malloc(pic_file[id].filelength);
		if(pic_file[id].grp ==NULL){
			JY_Error("JY_PicLoadFile: cannot malloc grp memory!\n");
			return 1;
		}
		fread(pic_file[id].grp,1,pic_file[id].filelength,fp);
		fclose(fp);
	}
	else{
		pic_file[id].fp=fp;
	}


	pic_file[id].pcache =(struct CacheNode **)malloc(pic_file[id].num*sizeof(struct CacheNode *));
	if(pic_file[id].pcache ==NULL){
		JY_Error("JY_PicLoadFile: cannot malloc pcache memory!\n");
		return 1;
	}

	for(i=0;i<pic_file[id].num;i++)
		pic_file[id].pcache[i]=NULL;

	if(percent == 0)
	{
		percent = (int)(g_Zoom*100);
	}
	pic_file[id].percent = percent;


	return 0;
} 

int JY_LoadPic(int fileid, int picid, int x,int y,int flag,int value)
{
	return JY_LoadPicColor(fileid, picid, x, y, flag, value, -1,0,0);
}

// ���ز���ʾ��ͼ
// fileid        ��ͼ�ļ�id 
// picid     ��ͼ���
// x,y       ��ʾλ��
//  flag ��ͬbit����ͬ���壬ȱʡ��Ϊ0
//  B0    0 ����ƫ��xoff��yoff��=1 ������ƫ����
//  B1    0     , 1 �뱳��alpla �����ʾ, value Ϊalphaֵ(0-256), 0��ʾ͸��
//  B2            1 ȫ��
//  B3            1 ȫ��
//  value ����flag���壬Ϊalphaֵ�� 

int JY_LoadPicColor(int fileid, int picid, int x, int y, int flag, int value, int pcolor, int nw, int nh)
{
 
	struct CacheNode *newcache,*tmpcache;
		int xnew,ynew;
		SDL_Surface *tmpsur;
		double zoom = 1;

		picid=picid/2;

		if(fileid<0 || fileid >=PIC_FILE_NUM || picid<0 || picid>=pic_file[fileid].num )    // ��������
			return 1;

		if(pic_file[fileid].pcache[picid]==NULL){   //��ǰ��ͼû�м���

			//����cache����
			newcache=(struct CacheNode *)malloc(sizeof(struct CacheNode));
			if(newcache==NULL){
				JY_Error("JY_LoadPic: cannot malloc newcache memory!\n");
				return 1;
			}

	        newcache->id =picid;
			newcache->fileid =fileid;
			LoadPic(fileid,picid,newcache);

			//ָ������
			zoom = (double)pic_file[fileid].percent/100.0;
			if(pic_file[fileid].percent > 0 && pic_file[fileid].percent != 100 && zoom != 0 && zoom != 1)
			{
				newcache->w = (int)(zoom * newcache->w);
				newcache->h = (int)(zoom * newcache->h);
				newcache->xoff = (int)(zoom * newcache->xoff);
				newcache->yoff = (int)(zoom * newcache->yoff);
			}

	        pic_file[fileid].pcache[picid]=newcache;

	        if(currentCacheNum<g_MAXCacheNum){  //cacheû��
	            list_add(&newcache->list ,&cache_head);    //���ص���ͷ
	            currentCacheNum=currentCacheNum+1;
	 		}
			else{   //cache ����
	            tmpcache=list_entry(cache_head.prev, struct CacheNode , list);  //���һ��cache
	            pic_file[tmpcache->fileid].pcache[tmpcache->id]=NULL;
				if(tmpcache->s!=NULL)
					SDL_DestroyTexture(tmpcache->s);       //ɾ������
				list_del(&tmpcache->list);
				SafeFree(tmpcache);

				list_add(&newcache->list ,&cache_head);    //���ص���ͷ
	            CacheFailNum++;
	            if(CacheFailNum % pic_file[tmpcache->fileid].bufflen ==1)
	                JY_Debug("Pic Cache is full!");
	        }
	    }
		else{   //�Ѽ�����ͼ
	 		newcache=pic_file[fileid].pcache[picid];
			list_del(&newcache->list);    //�����cache������ժ��
			list_add(&newcache->list ,&cache_head);    //�ٲ��뵽��ͷ
		}

		if(newcache->s==NULL){   //��ͼΪ�գ�ֱ���˳�
			return 1;
		}

		if(flag & 0x00000001){
			xnew=x;
			ynew=y;
		}
		else{
			xnew=x - newcache->xoff;
			ynew=y - newcache->yoff;
		}

		BlitSurface(newcache->s, xnew, ynew, newcache->w, newcache->h, flag, value, pcolor, nw, nh);

	    return 0;
}

int JY_LoadPicColor2(int fileid, int picid, int picid2, int x, int y, int flag, int value, int pcolor, int nw, int nh)
{

		SDL_RWops *fp_SDL;
		int id1,id2;
		int datalong;
		unsigned char *p,*data;
		SDL_Surface *tmps1,*tmps2;
		SDL_Texture	*tex;
		Uint32 g_MaskColor32=0x706020;      // ͸��ɫ
		Uint32 color=g_MaskColor32;//ConvertColor(g_MaskColor32);

		picid=picid/2;
		picid2=picid2/2;

		if(fileid<0 || fileid >=PIC_FILE_NUM || picid<0 || picid>=pic_file[fileid].num || picid2<0 || picid2>=pic_file[fileid].num )    // ��������
			return 1;

		if(pic_file[fileid].idx ==NULL){
			        JY_Error("LoadPic: fileid %d can not load?\n",fileid);
			        return 1;
		}
		id1=pic_file[fileid].idx[picid];
		id2=pic_file[fileid].idx[picid+1];
		if(id1<0)
			datalong=0;
		if(id2>pic_file[fileid].filelength)
			id2=pic_file[fileid].filelength;

		datalong=id2-id1;
				if(datalong>0){
					//��ȡ��ͼgrp�ļ����õ�ԭʼ����
			        if(g_PreLoadPicGrp==1){         //��Ԥ�������ڴ��ж�����
			            data=pic_file[fileid].grp+id1;
			            p=NULL;
			        }
			        else{       //û��Ԥ�������ļ��ж�ȡ
			            fseek(pic_file[fileid].fp,id1,SEEK_SET);
			            data=(unsigned char *)malloc(datalong);
			            p=data;
			            fread(data,1,datalong,pic_file[fileid].fp);
			        }
			        fp_SDL=SDL_RWFromMem(data,datalong);
			        if(IMG_isPNG(fp_SDL)==0){
			        	JY_Error("LoadPic: not png\n",fileid);
			        		        return 1;
					}
			        else{      //��ȡpng��ʽ
			        	tmps1=IMG_LoadPNG_RW(fp_SDL);
				        if(tmps1==NULL){
					        JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
				        }
			 		}
			        SDL_FreeRW(fp_SDL);
			        SafeFree(p);
			    }
			    else{
			    	JY_Error("LoadPic: fail!\n");
				}
				id1=pic_file[fileid].idx[picid2];
				id2=pic_file[fileid].idx[picid2+1];
				if(id1<0)
					datalong=0;
				if(id2>pic_file[fileid].filelength)
					id2=pic_file[fileid].filelength;

				datalong=id2-id1;
						if(datalong>0){
							//��ȡ��ͼgrp�ļ����õ�ԭʼ����
					        if(g_PreLoadPicGrp==1){         //��Ԥ�������ڴ��ж�����
					            data=pic_file[fileid].grp+id1;
					            p=NULL;
					        }
					        else{       //û��Ԥ�������ļ��ж�ȡ
					            fseek(pic_file[fileid].fp,id1,SEEK_SET);
					            data=(unsigned char *)malloc(datalong);
					            p=data;
					            fread(data,1,datalong,pic_file[fileid].fp);
					        }
					        fp_SDL=SDL_RWFromMem(data,datalong);
					        if(IMG_isPNG(fp_SDL)==0){
					        	JY_Error("LoadPic: not png\n",fileid);
					        		        return 1;
							}
					        else{      //��ȡpng��ʽ
					        	tmps2=IMG_LoadPNG_RW(fp_SDL);
						        if(tmps2==NULL){
							        JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
						        }
					 		}
					        SDL_FreeRW(fp_SDL);
					        SafeFree(p);
					    }
					    else{
					    	JY_Error("LoadPic: fail!\n");
						}

		//LoadPicSurface(fileid,picid,tmps1);
		//LoadPicSurface(fileid,picid2,tmps2);
		SDL_BlitSurface(tmps2,NULL,tmps1,NULL);
		SDL_SetColorKey(tmps1,SDL_TRUE ,color);
		tex=SDL_CreateTextureFromSurface(g_renderer,tmps1);
		BlitSurface(tex, x, y, tmps1->w, tmps1->h, flag, value, pcolor, nw, nh);
	    SDL_FreeSurface(tmps1);
	    SDL_FreeSurface(tmps2);
	    SDL_DestroyTexture(tex);
	    return 0;
}

// ������ͼ������
static int LoadPic(int fileid,int picid, struct CacheNode *cache)
{

	SDL_RWops *fp_SDL;
		int id1,id2;
		int datalong;
	    unsigned char *p,*data;
		SDL_Texture* tex = NULL;

	    if(pic_file[fileid].idx ==NULL){
	        JY_Error("LoadPic: fileid %d can not load?\n",fileid);
	        return 1;
	    }
	    id1=pic_file[fileid].idx[picid];
	    id2=pic_file[fileid].idx[picid+1];


		// ����һЩ��������������޸����еĴ���
		if(id1<0)
			datalong=0;

		if(id2>pic_file[fileid].filelength)
			id2=pic_file[fileid].filelength;

		datalong=id2-id1;


		if(datalong>0){
			//��ȡ��ͼgrp�ļ����õ�ԭʼ����
	        if(g_PreLoadPicGrp==1){         //��Ԥ�������ڴ��ж�����
	            data=pic_file[fileid].grp+id1;
	            p=NULL;
	        }
	        else{       //û��Ԥ�������ļ��ж�ȡ
	            fseek(pic_file[fileid].fp,id1,SEEK_SET);
	            data=(unsigned char *)malloc(datalong);
	            p=data;
	            fread(data,1,datalong,pic_file[fileid].fp);
	        }

	        fp_SDL=SDL_RWFromMem(data,datalong);
	        if(IMG_isPNG(fp_SDL)==0){
				int w,h;
	            w =*(short*)data;
	            h=*(short*)(data+2);
	            cache->w = w;
	            cache->h = h;
	            cache->xoff =*(short*)(data+4);
	            cache->yoff=*(short*)(data+6);
				cache->s=CreatePicSurface32(data+8,w,h,datalong-8);


			}
	        else{      //��ȡpng��ʽ
	        	int w, h;
	            tex=IMG_LoadTexture_RW(g_renderer,fp_SDL, 0);

		        if(tex==NULL){
			        JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
		        }
		        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
		        SDL_QueryTexture(tex, 0, 0, &w, &h);
		        cache->w = w;
		        cache->h = h;
	            cache->xoff=w/2;
	            cache->yoff=h/2;
	            cache->s=tex;


	 		}
	        SDL_FreeRW(fp_SDL);
	        SafeFree(p);


	    }
	    else{
			cache->s=NULL;
			cache->xoff=0;
			cache->yoff=0;
		}

	    return 0;
}

// ������ͼ������
static int LoadPicSurface(int fileid,int picid, SDL_Surface *tmps)
{
	    return 0;
}

//�õ���ͼ��С
int JY_GetPicXY(int fileid, int picid, int *w,int *h,int *xoff,int *yoff)
{
	struct CacheNode *newcache;
	int r=JY_LoadPic(fileid, picid, g_ScreenW+1,g_ScreenH+1,1,0);   //������ͼ����������λ��

	*w=0;
	*h=0;
	*xoff=0;
	*yoff=0;

	if(r!=0)
		return 1;

	newcache=pic_file[fileid].pcache[picid/2];

	if(newcache->s){      // ���У���ֱ����ʾ
		*w= newcache->w;
		*h= newcache->h;
		*xoff=newcache->xoff;
		*yoff=newcache->yoff;
	}

	return 0;
}




//����ԭ����Ϸ��RLE��ʽ��������
static SDL_Texture* CreatePicSurface32(unsigned char *data, int w,int h,int datalong)
{    
	int p=0;    
		int i,j;
		int yoffset;
		int row;
		int start;
	    int x;
	    int solidnum;
	    Uint32 *data32=NULL;
		SDL_Texture* tex = NULL;

	    data32=(Uint32 *)malloc(w*h*4);
		if(data32==NULL){
			JY_Error("CreatePicSurface32: cannot malloc data32 memory!\n");
			return NULL;
		}


		for(i=0;i<w*h;i++)
			data32[i]=0;



		tex = SDL_CreateTexture(g_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);



		for(i=0;i<h;i++){
	        yoffset=i*w;
	        row=data[p];            // i�����ݸ���
			start=p;
			p++;
			if(row>0){
				x=0;                // i��Ŀǰ��
	            for(;;){
	                x=x+data[p];    // i�пհ׵����������͸����
					if(x>=w)        // i�п�ȵ�ͷ������
						break;

					p++;
	                solidnum=data[p];  // ��͸�������
	                p++;
					for(j=0;j<solidnum;j++){
	                    data32[yoffset+x]=m_color32[data[p]] | 0xff000000;
						p++;
						x++;
					}
	                if(x>=w)
						break;     // i�п�ȵ�ͷ������
					if(p-start>=row)
						break;    // i��û�����ݣ�����
				}
	            if(p+1>=datalong)
					break;
			}
		}

		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

		SDL_UpdateTexture(tex, NULL, data32, w*4);

		if(tex == NULL)
		{
			JY_Error("CreatePicSurface32: cannot create SDL_CreateTextureFromSurface tex!\n");
		}
		SafeFree(data32);
	    return tex;
   	
}



// ��ȡ��ɫ��
// �ļ���Ϊ����ֱ�ӷ���
static int LoadPallette(char *filename)
{
    FILE *fp;
	char color[3];
	int i;
	if(strlen(filename)==0)    
		return 1;
	if((fp=fopen(filename,"rb"))==NULL){
        JY_Error("Pallette File not open ---%s",filename);
		return 1;
	}
	for(i=0;i<256;i++)
	{
         fread(color,1,3,fp);
         m_color32[i]=color[0]*4*65536l+color[1]*4*256+color[2]*4 ;
 
	}
	fclose(fp);

	return 0;
}


int JY_LoadPNGPath(const char* path, int fileid, int num, int percent, const char* suffix)
{
	int i;

	    struct CacheNode *tmpcache;
	    if(fileid<0 || fileid>=PIC_FILE_NUM){  // id������Χ
	        return 1;
		}

		if(pic_file[fileid].pcache){        //�ͷŵ�ǰ�ļ�ռ�õĿռ䣬������cache
			int i;
			for(i=0;i<pic_file[fileid].num;i++){   //ѭ��ȫ����ͼ��
	            tmpcache=pic_file[fileid].pcache[i];
	            if(tmpcache){       // ����ͼ�л�����ɾ��
				    if(tmpcache->s!=NULL)
					    SDL_DestroyTexture(tmpcache->s);       //ɾ������
				    list_del(&tmpcache->list);              //ɾ������
				    SafeFree(tmpcache);                  //�ͷ�cache�ڴ�
	                currentCacheNum--;
	            }
			}
	        SafeFree(pic_file[fileid].pcache);
	    }
		pic_file[fileid].num = num;
		sprintf(pic_file[fileid].path,"%s",path);

		pic_file[fileid].pcache =(struct CacheNode **)malloc(pic_file[fileid].num*sizeof(struct CacheNode *));
	    if(pic_file[fileid].pcache ==NULL){
			JY_Error("JY_LoadPNGPath: cannot malloc pcache memory!\n");
			return 1;
	    }
		for(i=0;i<pic_file[fileid].num;i++)
			pic_file[fileid].pcache[i]=NULL;


		pic_file[fileid].percent = percent;
		sprintf(pic_file[fileid].suffix,"%s",suffix);

		return 0;
}

int JY_LoadPNG(int fileid, int picid, int x,int y,int flag,int value, int px, int py, int pw, int ph)
{

	struct CacheNode *newcache,*tmpcache;
		SDL_Texture *tex = NULL;
		SDL_Rect rect1;
		SDL_Rect rect2;
		int w, h;

		picid = picid/2;

		if(fileid<0 || fileid >=PIC_FILE_NUM || picid<0 || picid>=pic_file[fileid].num )    // ��������
			return 1;

		if(pic_file[fileid].pcache[picid]==NULL){   //��ǰ��ͼû�м���
			char str[512];
			SDL_RWops *fp_SDL;
			double zoom = (double)pic_file[fileid].percent/100.0;

			sprintf(str,"%s/%d.%s",pic_file[fileid].path, picid,pic_file[fileid].suffix );

			//����cache����
			newcache=(struct CacheNode *)malloc(sizeof(struct CacheNode));
			if(newcache==NULL){
				JY_Error("JY_LoadPNG: cannot malloc newcache memory!\n");
				return 1;
			}

	        newcache->id =picid;
			newcache->fileid =fileid;

			fp_SDL=SDL_RWFromFile(str, "rb");
			if(IMG_isPNG(fp_SDL))
			{

				tex=IMG_LoadTexture_RW(g_renderer,fp_SDL,0);

		        if(tex==NULL){
			        JY_Error("JY_LoadPNG: cannot create SDL_Surface tmpsurf!\n");
			        return 1;
		        }
		        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
				SDL_QueryTexture(tex, 0, 0, &newcache->w, &newcache->h);
				newcache->xoff=newcache->w/2;
	            newcache->yoff=newcache->h/2;
	            newcache->s=tex;

	            //ָ������

				if(pic_file[fileid].percent > 0 && pic_file[fileid].percent != 100 && zoom != 0 && zoom != 1)
				{
					newcache->w = (int)(zoom * newcache->w);
					newcache->h = (int)(zoom * newcache->h);
					newcache->xoff = (int)(zoom * newcache->xoff);
					newcache->yoff = (int)(zoom * newcache->yoff);
				}

			}
			else
			{
				JY_Error("JY_LoadPNG: is not png!\n");
				newcache->s=NULL;
				newcache->xoff=0;
				newcache->yoff=0;
			}

			SDL_FreeRW(fp_SDL);


			
			pic_file[fileid].pcache[picid]=newcache;

	    if(currentCacheNum<g_MAXCacheNum){  //cacheû��
	            list_add(&newcache->list ,&cache_head);    //���ص���ͷ
	            currentCacheNum=currentCacheNum+1;
	 		}
			else{   //cache ����
	            tmpcache=list_entry(cache_head.prev, struct CacheNode , list);  //���һ��cache
	            pic_file[tmpcache->fileid].pcache[tmpcache->id]=NULL;
				if(tmpcache->s!=NULL)
					SDL_DestroyTexture(tmpcache->s);       //ɾ������
				list_del(&tmpcache->list);
				SafeFree(tmpcache);

				list_add(&newcache->list ,&cache_head);    //���ص���ͷ
	            CacheFailNum++;
	            if(CacheFailNum % 100 ==1)
	                JY_Debug("Pic Cache is full!");
	        }
	    }
		else{   //�Ѽ�����ͼ
	 		newcache=pic_file[fileid].pcache[picid];
			list_del(&newcache->list);    //�����cache������ժ��
			list_add(&newcache->list ,&cache_head);    //�ٲ��뵽��ͷ
		}

		if(newcache->s==NULL){   //��ͼΪ�գ�ֱ���˳�
			return 1;
		}

		if(flag & 0x1){

		}
		else{
			x -= newcache->xoff;
			y -= newcache->yoff;
		}

		rect1.x=x;
		rect1.y=y;

		rect1.w = newcache->w;
		rect1.h = newcache->h;

		SDL_QueryTexture(newcache->s, 0, 0, &w, &h);
		rect2.x = 0;
		rect2.y = 0;
		rect2.w = w;
		rect2.h = h;


		if(pw >= 0 && ph >= 0){

			double zoom = (double)pic_file[fileid].percent/100.0;
			rect2.x = px;
			rect2.y = py;
			rect2.w = (int)(pw/zoom);
			rect2.h = (int)(ph/zoom);

			rect1.w = pw-px;
			rect1.h = ph-py;

		}

		if((flag & 0x2)==0){        // û��alpla

		}
		else{
			SDL_SetTextureAlphaMod(newcache->s,(Uint8)value);
		}

		SDL_RenderCopy(g_renderer, newcache->s, &rect2, &rect1);

		//BlitSurface(newcache->s, x, y, newcache->w, newcache->h, 0,0, 0);



		return 0;
}


int JY_GetPNGXY(int fileid, int picid, int *w,int *h,int *xoff,int *yoff)
{
	struct CacheNode *newcache;
	int r=JY_LoadPNG(fileid, picid, g_ScreenW+1,g_ScreenH+1,1,0,0,0,0,0);   //������ͼ����������λ��

	*w=0;
	*h=0;
	*xoff=0;
	*yoff=0;

	if(r!=0)
		return 1;

	newcache=pic_file[fileid].pcache[picid/2];

	if(newcache->s){      // ���У���ֱ����ʾ
		//int ww, hh;
		//SDL_QueryTexture(newcache->s, 0, 0, &ww, &hh);
		*w= newcache->w;
		*h= newcache->h;
		*xoff=newcache->xoff;
		*yoff=newcache->yoff;
	}

	return 0;
}

struct CacheNode* GetPicCache(int fileid, int picid)
{
	return pic_file[fileid].pcache[picid/2];
}
