/*
 * main.cpp
 *
 *  Created on: Mar 21, 2019
 *      Author: gilg
 */

#include <math.h>


#include <unistd.h>
#include <sys/time.h>

#include <cstdio>
#include <GL/glut.h>

typedef struct
{
	double x;
	double y;
}vertices2d;

suseconds_t differe(timeval tim1,timeval tim2)
{return (tim2.tv_sec - tim1.tv_sec)*1E6+tim2.tv_usec-tim1.tv_usec;}

double Lx=0.04;//0.02;
double Ly=0.04;
double T=2;

double E=2*10e11/10000000000;
double nu=0.33;
double rho=7640;

double hx=0.0005;
double hy=0.0005;
double tay=0.0002;

double lambda=nu*E/(1+nu)/(1-2*nu);
double mu=E*2/(1+nu);



unsigned int Nx=Lx/hx;
unsigned int Ny=Ly/hy;
unsigned int Nt=T/tay;


double omega_r (double x)
{
	return 0;
}
double omega_l(double y)
{
	return Lx;
}
//boundary conditions------------------------------------------
double bottom (double T, double y){return 0;}
double top (double T, double y){return 0;}
//-------------------------------------------------------------
//initial conditions-------------------------------------------
inline double fu(double x,double y){return 0;}
inline double fu1(double x,double y){return 0;}
inline double fv(double x,double y){return 0;}
inline double fv1(double x,double y){return 0;}
//-------------------------------------------------------------
double ***u;
double ***v;


vertices2d* line;
unsigned int* ptr;

void pressNormalKeys(unsigned char key, int xx, int yy)
{
        switch (key)
        {
              case 0x1b:
              {

            	  //for(int k=0;k<R;k++){free(u[k]);}
            	  //free(u);
            	  exit(255);}
        }
}




void display()// Функция перерисовки дисплея
{
	glClearColor(1.0, 1.0,1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	 glLoadIdentity();


	 static  unsigned int k=0;

/*
	 line[0].x=0;
	 line[0].y=0;
	 line[1].x=hx;
	 line[1].y=0;
	 line[8].x=hx;
	 line[8].y=hy;
	 line[7].x=0;
	 line[7].y=hy;
*/

	 GLubyte color[(Nx+1)*(Ny+1)][3];

	 double mashtab=512.0/sqrt(u[0][Nx/2][Ny/2]*u[0][Nx/2][Ny/2]+v[0][Nx/2][Ny/2]*v[0][Nx/2][Ny/2]);
	 static uint32_t color_max=0;

	 for(unsigned int n=0;n<(Nx+1)*(Ny+1);n++)
	 {
		 unsigned int j=n/(Nx+1);
		 unsigned int i=n-j*(Nx)-j;
		 //uint32_t bias=0;
		 uint32_t color_=(uint32_t)(mashtab*sqrt(u[k][i][j]*u[k][i][j]+v[k][i][j]*v[k][i][j]));

		 if((color_>=0)&&(color_<128))
		 {
			 color[n][0]=0;//r
			 color[n][1]=0;//g
			 color[n][2]=color_;//b
		 }
		 if((color_>=128)&&(color_<255))
		 {
			 color[n][0]=0;//b
			 color[n][1]=color_-128;//g
			 color[n][2]=255-color_;//r
		 }
		 if((color_>=255)&&(color_<384))
		 {
			 color[n][0]=color_-255;//b
			 color[n][1]=384-color_;//g
			 color[n][2]=0;//r
		 }
		 if((color_>=384)&&(color_<512))
		 {
			 color[n][0]=color_-384;//r
			 color[n][1]=0;//g
			 color[n][2]=0;//b
		 }
		 if(color_>512)
		 {
			 color[n][0]=255;//r
			 color[n][1]=0;//g
			 color[n][2]=0;//b
		 }


		 if((color[n][0]!=0)||(color[n][1]!=0)||(color[n][2]!=0))
		 {
			 //printf("%u %f  %f R%3hhu G%3hhu B%3hhu k%3i  %u \r\n",n,line[n].x,line[n].y,color[n][0],color[n][1],color[n][2],k,color_max);
		 }
		 if(color_>color_max){color_max=color_;}
	 }

	 k++;
	 if(k>Nt){exit(255);}
	 usleep(10000);
	 glColorPointer(3, GL_UNSIGNED_BYTE, 0, color);
	 glVertexPointer(2, GL_DOUBLE, 0, line);


	 glEnableClientState(GL_VERTEX_ARRAY);
	 glEnableClientState(GL_COLOR_ARRAY);

	 //glDrawArrays(GL_QUADS,0,16);
	 glDrawElements(GL_QUADS,Nx*Ny*4, GL_UNSIGNED_INT, ptr);

	 glutSwapBuffers();
}

void opengl_init(int argc, char** argv)
{
	  glutInit(&argc, argv);
	  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	  glutInitWindowSize(1920, 1080);
	  glutInitWindowPosition(0, 0);
	  glutCreateWindow("graf");
	  glClearColor(0, 0, 0, 0);
	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
	  glOrtho(0,Lx, 0, Ly, -100, 100);
	  glutDisplayFunc(display);
	  glutIdleFunc(display);
	  glMatrixMode(GL_MODELVIEW);

	  glutKeyboardFunc(pressNormalKeys);


	  glutMainLoop();
}


void print_m(double **ptr)
{
	for(unsigned int j=0;j<Ny+1;j++)
	{
		for(unsigned int i=0;i<Nx+1;i++){printf(" %6.3f",ptr[i][j]);}
		printf("\r\n");
	}
}

void print_m_i(double **ptr)
{
	for(int j=Ny-1;j>=0;j--)
	{
		for(unsigned int i=0;i<Nx;i++){printf(" %6.3f",ptr[i][j]);}
		printf("\r\n");
	}

}

void f(unsigned int *ptr)
{
	for(unsigned int i=0;i<Nx*Ny;i++)
	{
		ptr[4*i]=(i+i/Nx);
		ptr[4*i+1]=(i+i/Nx)+1;
		ptr[4*i+2]=(i+i/Nx)+Nx+2;
		ptr[4*i+3]=(i+i/Nx)+Nx+1;
	}
}

void allocate_memory(double ****u)
{
	*u=(double***)malloc(sizeof(double***)*(Nt+1));
	for(unsigned int k=0;k<(Nt+1);k++)
	{
		(*u)[k]=(double**)malloc(sizeof(double**)*(Nx+1));
		for(unsigned int i=0;i<(Nx+1);i++){(*u)[k][i]=(double*)malloc(sizeof(double*)*(Ny+1));}
	}
}

void deallocate_memory(double ****u )
{
	for(unsigned int k=0;k<((Nt+1));k++)
	{
		for(unsigned int i=0;i<(Nx+1);i++){free((*u)[k][i]);}free((*u)[k]);
	}
	free((*u));
}


int main(int argc, char** argv)
{
	printf("number of nodes %i  %i  %i\r\n",Nx,Ny,Nt);
	printf("allocate memory %iMB\r\n",2*Nx*Ny*Nt*8/1000000);


	//return 1;
	//allocating arrays---------------------------------------------------------------
	allocate_memory(&u);
	allocate_memory(&v);

	//------------------------------------------------------------------------------------
	double a = (lambda + 2*mu)*tay*tay/rho;
	double b = mu*tay*tay/rho;
	double c = (lambda + mu)*tay*tay/(hy*hx*rho)/4;

	double au = a/(hx*hx);
	double av = a/(hy*hy);
	double bu = b/(hy*hy);
	double bv = b/(hx*hx);
	double cu = c;
	double cv = c;
	double du=2*(1 - au - bu);
	double dv=2*(1 - av - bv);


	double b1u=lambda*(tay*tay)/((hy*hy)*rho);
	double b1v=lambda*(tay*tay)/((hx*hx)*rho);
	double c1u=2*(1-au+b1u);
	double c1v=2*(1-av+b1v);

	printf("au %f  av %f bu %f bv %f cu %f cv %f \r\n",sqrt(au),sqrt(av),sqrt(bu),sqrt(bv),sqrt(cu),sqrt(cv));
	//inserting inital condition ---------------------------------------------------------

	for (unsigned int  i=0;i<Nx+1;i++)
	{for(unsigned int  j=0;j<Ny+1;j++){u[0][i][j]=fu(hx*i,hy*j);}}//u

	for (unsigned int  i=0;i<Nx+1;i++)
	{for(unsigned int  j=0;j<Ny+1;j++){v[0][i][j]=fv(hx*i,hy*j);}}//v


	u[0][Nx/2][Ny/2]=0.02;
	//second type-----------------------------------------------------------------------------

	//solution the problem--------------------------------------------------------------------
	for(unsigned int  k=1;k<Nt;k++)
	{
		//boundary condition --------------------------------------------------------
		for(unsigned  int i=0;i<Nx+1;i++)
		{u[k+1][i][0]=0;v[k+1][i][0]=0;}
		//-------------------------------------------------------------------------------
		for(unsigned  int j=1;j<Ny;j++)
		{
			//boundary condition --------------------------------------------------------
			u[k+1][0][j]=0;v[k+1][0][j]=0;
/*
			int i=0;
			double u_k_i_1_j=(lambda*hx)*(v[k][i][j+1]-v[k][i][j-1])/((lambda+2*mu)*hy)+u[k][i+1][j];//u[k][i-1][j] i=0
			double v_k_i_1_j =((u[k][i][j+1]-u[k][i][j-1])*hx)/hy+v[k][i+1][j];//v[k][i-1][j]  i=0

			u[k+1][i][j]=
					(u[k][i+1][j]+u_k_i_1_j)*au
					+(u[k][i][j+1]+u[k][i][j-1])*b1u
					+u[k][i][j]*c1u
					-u[k-1][i][j];

			v[k+1][i][j]=
					(v[k][i][j+1]+v[k][i][j-1])*av
					+(v[k][i+1][j]+v_k_i_1_j)*b1v
					+v[k][i][j]*c1v
					-v[k-1][i][j];
*/
			//-------------------------------------------------------------------------------

			for(unsigned  int i=1;i<Nx;i++)
			{

				u[k+1][i][j]=
						(u[k][i+1][j]+u[k][i-1][j])*au
						+(u[k][i][j+1]+u[k][i][j-1])*bu
						+(v[k][i+1][j+1]-v[k][i+1][j-1]-v[k][i-1][j+1]+v[k][i-1][j-1])*cu
						+u[k][i][j]*du
						-u[k-1][i][j];


				v[k+1][i][j]=
						(v[k][i][j+1]+v[k][i][j-1])*av
						+(v[k][i+1][j]+v[k][i-1][j])*bv
						+(u[k][i+1][j+1]-u[k][i+1][j-1]-u[k][i-1][j+1]+u[k][i-1][j-1])*cv
						+v[k][i][j]*dv
						-v[k-1][i][j];

			}
			//boundary condition --------------------------------------------------------
			u[k+1][Nx][j]=0;v[k+1][Nx][j]=0;

/*
			 i =Nx;
			double u_k_ip1_j=-((lambda*hx)*(v[k][i][j+1]-v[k][i][j-1])/((lambda+2*mu)*hy))+u[k][i-1][j];//u[k][i+1][j] i=Nx
			double v_k_ip1_j=-((u[k][i][j+1]-u[k][i][j-1])*hx)/hy+v[k][i-1][j];//v[k][i+1][j]	i=Nx

			u[k+1][i][j]=
					(u_k_ip1_j+u[k][i-1][j])*au
					+(u[k][i][j+1]+u[k][i][j-1])*b1u
					+u[k][i][j]*c1u
					-u[k-1][i][j];

			v[k+1][i][j]=
					(v[k][i][j+1]+v[k][i][j-1])*av
					+(v_k_ip1_j+v[k][i-1][j])*b1v
					+v[k][i][j]*c1v
					-v[k-1][i][j];
*/

			//-------------------------------------------------------------------------------

		}
		//boundary condition --------------------------------------------------------
		for(unsigned  int i=0;i<Nx+1;i++)
		{u[k+1][i][Ny]=0;v[k+1][i][Ny]=0;}
		//--------------------------------------------------------------------------------------------
	}

/*
	for(unsigned int  k=0;k<30;k++)
	{
		print_m(u[k]);
		printf("----------------------------------------------- %u\r\n",k);
	}
*/


	ptr=(unsigned int*) malloc(sizeof(unsigned int)*Nx*Ny*4);
	f(ptr);

	line=(vertices2d *)malloc(sizeof(vertices2d )*(Nx+1)*(Ny+1));

	 for(unsigned int n=0;n<(Nx+1)*(Ny+1);n++)
	 {
		 unsigned int j=n/(Nx+1);
		 unsigned int i=n-j*(Nx)-j;
		 line[n].x=i*hx;
		 line[n].y=j*hy;
		 //printf("%u %f  %f \r\n",n,line[n].x,line[n].y);
	 }
/*
	 for(unsigned int n=0;n<Nx*Ny*4;n+=4)
	 {

		 printf("%u %f  %f  %f  %f \r\n"
				"%u %f  %f  %f  %f %u %u %u %u\r\n",n,line[ptr[n]].x,line[ptr[n+1]].x,line[ptr[n+2]].x,line[ptr[n+3]].x,
													n,line[ptr[n]].y,line[ptr[n+1]].y,line[ptr[n+2]].y,line[ptr[n+3]].y,ptr[n],ptr[n+1],ptr[n+2],ptr[n+3]);
	 }
*/
	opengl_init(argc, argv);
	//deallocating arrays--------------------------------------------------------------
	 free(line);


	 deallocate_memory(&u);
	 deallocate_memory(&v);
	//---------------------------------------------------------------------------------


	return 0xff;
}



