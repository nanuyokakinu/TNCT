#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_data( char data[512], FILE *rfp);
void printimg( char data[512]);
void expand( char data[512], char pattern [64][64]);
void clear_ry( char data[512]);
void compress( char data[512], char pattern[64][64]);
void outline( char p[64][64]);
void smooth( char p[64][64]);
void normalize( char p[64][64]);
int label( char p[64][64]);
void noise( char p[64][64], int size);
void extract( char p[64][64], FILE *wfp);
void ext16( char p[64][64], int i, int j, int feat16[4]);

int main( int argc, char *argv[] ) {
	int i, j, k, num;
	char data[512],pattern[64][64],fn[0x80], *tmp ;
	FILE *rfp, *wfp;
	
	for( i = 1 ; i < argc ; i++ ) {
		strcpy( fn, argv[i] ) ;
		for( j = strlen(fn)-1 ; j >= 0 ; j-- ) {
			if( fn[j] == '.' ){
				fn[j]='\0';
				break ;
			}
		}
		strcat( fn, ".ftr" ) ;
		j=1;
		wfp = fopen( fn, "w");
		tmp = strrchr( fn, 'f');
		fn[tmp-fn]='i';
		fn[tmp-fn+1]='m';
		fn[tmp-fn+2]='g';
		rfp = fopen( fn, "rb" ) ;
		fseek( rfp, 0, SEEK_END ) ;
		num = ftell( rfp ) / 512 ;
		fseek( rfp, 0, SEEK_SET ) ;
		for (k=0;k<num;k++){
			read_data(data,rfp);
			expand(data,pattern); 
			clear_ry(data);
			label(pattern);
			noise(pattern,4);
			smooth(pattern);
			normalize(pattern);
			outline(pattern);
			extract(pattern,wfp);
		}
		fclose(wfp);
		fclose(rfp);
	}
}

//dataに画像データを読み込む
void read_data(char data[512], FILE *rfp) 
{
	if(fread(data,sizeof(char),512,rfp)!=512){
		fprintf(stderr,"cannot read \n");
		exit(1);
	}
}

//1文字分の画像データを受け取り，画面に表示する関数
void printimg(char data[512])
{
	int i,j,k,b;
	for(k=0;k<64;k++){
		for(i=k*8;i<k*8+8;i++){
			b=0x80;
			for(j=0;j<8;j++){
				if((data[i]&b)==0) printf(".");
				else printf("*");
				b=b>>1;
			}
		}
		printf("\n");
	}
}

//1文字分の画像データを受け取り，char型の2次元配列に展開して保存する関数
void expand(char data[512],char pattern [64][64])
{
	int i,j,k,b,l;
	for(k=0;k<64;k++){
		l=0;
		for(i=k*8;i<k*8+8;i++){
			b=0x80;
			for(j=0;j<8;j++){
				if((data[i]&b)==0) pattern[k][l]=0;
				else pattern[k][l]=1;
				b=b>>1;
				l++;
			}
		}
	}
}

//expandの逆操作
void compress(char data[512],char pattern[64][64])
{
	int i,j;
	unsigned char m;
	for(i=0;i<64;i++){
		for(j=0;j<64;j++){
			if(pattern[i][j]==1){
				m = 128;
				m = m>>j%8;
				data[(i*64+j)/8] += m;
			}
		}
	}
}

//dataをクリアする
void clear_ry(char data[512])
{
	int i;
	for(i=0;i<512;i++) data[i]='\0';
}

//pの輪郭抽出を行う
void outline(char p[64][64])
{
	int i,j;
	for(i=1;i<63;i++){
		for(j=1;j<63;j++){
			if(p[i][j]==1){
				if(p[i][j-1]*p[i-1][j]*p[i][j+1]*p[i+1][j]!=0) p[i][j]=2;
			}
		}
	}
	for(i=1;i<63;i++){
		for(j=1;j<63;j++){
			if(p[i][j]==2) p[i][j]=0;
		}
	}
}

//pにスムージング処理を行う
void smooth(char p[64][64])
{
	char mask[3][3]={{0,1,0},{1,1,1},{1,1,1}};
	int i,j,turn_count,rec[400][2]={0},recnum=0,flag=1;
	
	while(flag){
		mask[1][0]=1,mask[0][0]=0,mask[0][2]=0;
		for(turn_count=0;turn_count<4;turn_count++){
			for(i=1;i<63;i++){
				for(j=1;j<63;j++){
					if(p[i][j]==1){
						if (p[i-1][j-1]==mask[0][0] && p[i-1][j]==mask[0][1] && p[i-1][j+1]==mask[0][2] &&
							p[i][j-1]==mask[1][0]   && 							p[i][j+1]==mask[1][2]&&
							p[i+1][j-1]==mask[2][0] && p[i+1][j]==mask[2][1] && p[i+1][j+1]==mask[2][2]){
							if(turn_count==0){
								rec[recnum][0]=i-1;
								rec[recnum][1]=j;
								recnum++;
							} else if (turn_count==1){
								rec[recnum][0]=i;
								rec[recnum][1]=j+1;
								recnum++;
							} else if (turn_count==2){
								rec[recnum][0]=i+1;
								rec[recnum][1]=j;
								recnum++;
							} else if (turn_count==3){
								rec[recnum][0]=i;
								rec[recnum][1]=j-1;
								recnum++;
							} 
						}
					}
				}
			}
			if(turn_count==0){
				mask[0][0]=1,mask[2][2]=0;
			} else if (turn_count==1){
				mask[0][2]=1,mask[2][0]=0;
			} else if (turn_count==2){
				mask[2][2]=1,mask[0][0]=0;
			}
		}
		if(recnum==0) flag=0;
		while(recnum>0){
			p[rec[recnum-1][0]][rec[recnum-1][1]]=0;
			recnum--;
		}

		mask[2][0]=1,mask[0][0]=1,mask[0][1]=0;
		for(turn_count=0;turn_count<4;turn_count++){
			for(i=1;i<63;i++){
				for(j=1;j<63;j++){
					if(p[i][j]==1){
						if (p[i-1][j-1]==mask[0][0] && p[i-1][j]==mask[0][1] && p[i-1][j+1]==mask[0][2] &&
							p[i][j-1]==mask[1][0]   && 							p[i][j+1]==mask[1][2]&&
							p[i+1][j-1]==mask[2][0] && p[i+1][j]==mask[2][1] && p[i+1][j+1]==mask[2][2]){
							if(turn_count==0){
								rec[recnum][0]=i-1;
								rec[recnum][1]=j;
								recnum++;
							} else if (turn_count==1){
								rec[recnum][0]=i;
								rec[recnum][1]=j+1;
								recnum++;
							} else if (turn_count==2){
								rec[recnum][0]=i+1;
								rec[recnum][1]=j;
								recnum++;
							} else if (turn_count==3){
								rec[recnum][0]=i;
								rec[recnum][1]=j-1;
								recnum++;
							} 
						}
					}
				}
			}
			if(turn_count==0){
				mask[0][1]=1,mask[1][2]=0;
			} else if (turn_count==1){
				mask[1][2]=1,mask[2][1]=0;
			} else if (turn_count==2){
				mask[2][1]=1,mask[1][0]=0;
			}
		}
		if(flag==0&&recnum!=0) flag=1;
		while(recnum>0){
			p[rec[recnum-1][0]][rec[recnum-1][1]]=1;
			recnum--;
		}
	}
}

//正規化を行う
void normalize( char p[64][64] )
{
	int i,j,x0,y0,x1,y1,w,h,u,v;
	char tmp[64][64]={0};
	for(i=0;i<64;i++){
		for(j=0;j<64;j++){
			if(p[i][j]==1){
				y0=i;
				j+=64;
				i+=64;
			}
		}
	}
	for(j=0;j<64;j++){
		for(i=0;i<64;i++){
			if(p[i][j]==1){
				x0=j;
				j+=64;
				i+=64;
			}
		}
	}
	for(i=0;i<64;i++){
		for(j=0;j<64;j++){
			if(p[64-i][64-j]==1){
				y1=64-i;
				j+=64;
				i+=64;
			}
		}
	}	
	for(j=0;j<64;j++){
		for(i=0;i<64;i++){
			if(p[64-i][64-j]==1){
				x1=64-j;
				j+=64;
				i+=64;
			}
		}
	}
	w=x1-x0+1;
	h=y1-y0+1;
	if(w > h){
		y0=y0-(w-h)/2;
		h=w;
	} else if(w < h) {
		x0=x0-(h-w)/2;
		w=h;
	}
	for(u=0;u<64;u++){
		for(v=0;v<64;v++){
			if(p[(int)( u * ( w / 64.0  ) + y0 + 0.5)][(int)( v * ( h / 64.0 ) + x0 + 0.5 )]==1) tmp[u][v]=1;
		}
	}
	for(i=0;i<64;i++){
		for(j=0;j<64;j++){
			p[i][j]=tmp[i][j];
		}
	}
}

//ラベリング処理
int label( char p[64][64] )
{
	int y=0,x=0,stack[2048][2],stack_num,label_num=1,i,j;
	while(y<64 && x<64){
		if(p[y][x]==1){
			label_num++;
			p[y][x]=label_num;
			stack[0][0]=y;
			stack[0][1]=x;
			stack_num=1;
			while(stack_num){
				if(p[ stack[stack_num-1][0] -1 ][ stack[stack_num-1][1] -1 ]==1 && 0<stack[stack_num-1][0] && 0<stack[stack_num-1][1] ){
					p[ stack[stack_num-1][0] -1 ][ stack[stack_num-1][1] -1 ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] -1 ;
					stack[stack_num][1]=stack[stack_num-1][1] -1 ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] -1 ][ stack[stack_num-1][1] ]==1 && 0<stack[stack_num-1][0] ){
					p[ stack[stack_num-1][0] -1 ][ stack[stack_num-1][1] ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] -1 ;
					stack[stack_num][1]=stack[stack_num-1][1] ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] -1 ][ stack[stack_num-1][1] +1 ]==1 && 0<stack[stack_num-1][0] && stack[stack_num-1][1]<63 ){
					p[ stack[stack_num-1][0] -1 ][ stack[stack_num-1][1] +1 ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] -1 ;
					stack[stack_num][1]=stack[stack_num-1][1] +1 ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] ][ stack[stack_num-1][1] -1 ]==1 && 0<stack[stack_num-1][1] ){
					p[ stack[stack_num-1][0] ][ stack[stack_num-1][1] -1 ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] ;
					stack[stack_num][1]=stack[stack_num-1][1] -1 ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] ][ stack[stack_num-1][1] +1 ]==1 && stack[stack_num-1][1]<63 ){
					p[ stack[stack_num-1][0] ][ stack[stack_num-1][1] +1 ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] ;
					stack[stack_num][1]=stack[stack_num-1][1] +1 ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] +1 ][ stack[stack_num-1][1] -1 ]==1 && stack[stack_num-1][0]<63 && 0<stack[stack_num-1][1] ){
					p[ stack[stack_num-1][0] +1 ][ stack[stack_num-1][1] -1 ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] +1 ;
					stack[stack_num][1]=stack[stack_num-1][1] -1 ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] +1 ][ stack[stack_num-1][1] ]==1 && stack[stack_num-1][0]<63 ){
					p[ stack[stack_num-1][0] +1 ][ stack[stack_num-1][1] ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] +1 ;
					stack[stack_num][1]=stack[stack_num-1][1] ;
					stack_num++;
					continue;
				}
				if(p[ stack[stack_num-1][0] +1 ][ stack[stack_num-1][1] +1 ]==1 && stack[stack_num-1][0]<63 && stack[stack_num-1][1]<63 ){
					p[ stack[stack_num-1][0] +1 ][ stack[stack_num-1][1] +1 ] = label_num;
					stack[stack_num][0]=stack[stack_num-1][0] +1 ;
					stack[stack_num][1]=stack[stack_num-1][1] +1 ;
					stack_num++;
					continue;
				}
				stack_num--;
			}
		}
		if(label_num>255) return 1;
		x++;
		if(x%64==0){
			y++;
			x=0;
		}
	}
	return 0;
}

//ノイズ除去
void noise( char p[64][64], int size )
{
	int i,j,k,count,noise_list[255],noise_num=0,find_num=0;
	while(count!=0){
		count=0;
		for(i=0;i<64;i++){
			for(j=0;j<64;j++){
				if(p[i][j]==2+find_num) count++;
			}
		}
		if(count<=size){
			noise_list[noise_num]=2+find_num;
			noise_num++;
		}
		find_num++;
	}
	for(k=0;k<noise_num;k++){
		for(i=0;i<64;i++){
			for(j=0;j<64;j++){
				if(p[i][j]==noise_list[noise_num]) p[i][j]=0;
			}
		}
	}
	for(i=0;i<64;i++){
		for(j=0;j<64;j++){
			if( p[i][j]>1 ) p[i][j]=1;
		}
	}
}

//特徴量抽出
void extract( char p[64][64], FILE *wfp)
{
	int i,j,k,feature[4],feat16[4]={0};
	for(i=0;i<7;i++){
		for(j=0;j<7;j++){
			ext16(p,i*8,j*8,feat16);
			for(k=0;k<4;k++){
				fprintf(wfp, "%d\n", feat16[k] );
			}
			for(int l=0;l<4;l++){
				feat16[l]=0;
			}
		}
	}
}

void ext16( char p[64][64], int i, int j, int feat16[4])
{
	int l,m;
	for(l=0;l<16;l+=2){
		for(m=0;m<16;m+=2){
			if(p[i+l][j+m]*p[i+l+1][j+m]==1) feat16[0]+=1;
			//if(p[i+l][j+m+1]*p[i+l+1][j+m+1]==1) feat16[0]+=1;
			if(p[i+l][j+m]*p[i+l][j+m+1]==1) feat16[1]+=1;
			//if(p[i+l+1][j+m]*p[i+l+1][j+m+1]==1) feat16[1]+=1;
			if(p[i+l][j+m]*p[i+l+1][j+m+1]==1) feat16[2]+=1;
			if(p[i+l+1][j+m]*p[i+l][j+m+1]==1) feat16[3]+=1;
		}
	}
	for(l=1;l<=15;l+=2){
		for(m=1;m<=15;m+=2){
			if(p[i+l][j+m]*p[i+l+1][j+m]==1) feat16[0]+=1;
			//if(p[i+l][j+m+1]*p[i+l+1][j+m+1]==1) feat16[0]+=1;
			if(p[i+l][j+m]*p[i+l][j+m+1]==1) feat16[1]+=1;
			//if(p[i+l+1][j+m]*p[i+l+1][j+m+1]==1) feat16[1]+=1;
			if(p[i+l][j+m]*p[i+l+1][j+m+1]==1) feat16[2]+=1;
			if(p[i+l+1][j+m]*p[i+l][j+m+1]==1) feat16[3]+=1;
		}
	}
}




















