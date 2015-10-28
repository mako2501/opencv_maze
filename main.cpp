#include <opencv.hpp>
#include <highgui/highgui_c.h>
#include <iostream>
#include <cv.h>

/*
NAI - PJATK
OpenCV Projekt - Labirynt
Program odczytuje z kartki labirynt, interpretuje odpowiednio pola, znajduje start i wyszukuje wyjście.
#2013
*/

using namespace std;
using namespace cv;

// kierunki ruchu Tezeusza
enum direction { North, East, South, West };

// zwraca prostokat->kontur najwiekszego bloba z mattresha -> parametr
Rect biggestContour(Mat thr){

			int largest_area=0;
			int largest_contour_index=0;
			Rect bounding_rect;
			vector<vector<Point>> contours; // wektor do przechowania konturow http://docs.opencv.org/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html?highlight=findcontours#findcontours
			vector<Vec4i> hierarchy;
			
			findContours( thr, contours, hierarchy,CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE ); // szuka wszystkich konturow w obrazie
			 for( int i = 0; i< contours.size(); i++ ) // po kazdym 
					  {
					   double a=contourArea( contours[i],false);  //  oblicza powieszchniê obszaru
					   if(a>largest_area){
					   largest_area=a;
					   largest_contour_index=i;                //zachowaj index jesli jest to wiekszy obszar
					   bounding_rect=boundingRect(contours[i]); // zbuduj bounding rec
					   }   
			}
			 return bounding_rect;
}

// rysuje zielon¹ liniê w macierzy pomiedzy dwoma punktami
void drawLine( Mat img, Point start, Point end )
{
  int thickness = 2;
  int lineType = 8;
  line( img,
        start,
        end,
        Scalar( 0, 255, 0 ),
        thickness,
        lineType );
}

//sprawdza czy punkt jest poza prost. - labiryntem
bool isOutside(Point p,Rect r){
	cout<<"isOutside point=["<<p<<"], is "<<(!r.contains(p))<<"\n";
	return (!r.contains(p));

}

// sprawdza czy w zadanym kierunku po kroku jest sciana
bool wallExist(Mat m, Point t, int step, direction d){

	Point p = t;
	switch(d){
	case North: p.y-=step;break;
	case South: p.y+=step;break;
	case West: p.x-=step;break;
	case East: p.x+=step;break;
	}
	//cout<<"wallExist checkDir=["<<d<<"], point=["<<p<<"], isExist "<<(m.at<uchar>(p.y,p.x) == 0)<<"\n";
	// uwaga na punkt poza macierz¹
	if(p.x>=0 && p.y>=0 && p.x<=m.cols && p.y<=m.rows)
	{
	Point3_<uchar>* c = m.ptr<Point3_<uchar> >(p.y,p.x);
	// if(m.at<uchar>(p.y,p.x) == 0) return true;
		if(c->x == 0)return true;
	 else
		 return false;
	}
	return false;
}


// zwraca punkt nastepnego prawidlowego kroku
Point addPoint(Mat m, Point t, int step,direction d){
	
	Point p = t;
	switch(d){
	case North: p.y-=step;break;
	case South: p.y+=step;break;
	case West: p.x-=step;break;
	case East: p.x+=step;break;
	}
	//drawLine(m,p,p);
	cout<<"addPoint point=["<<p<<"]\n";
	return p;
}

//wypala miejsce - tu by³em
void mark(Mat m, Point p,int step)
{
	cout<<"mark point=["<<p<<"]\n";
	rectangle(m,Point(p.x-step/2+2,p.y-step/2+2),Point(p.x+step/2-2, p.y+step/2-2),Scalar(125,125,125),CV_FILLED );
}

// odznacza miejce tu by³em
void unMark(Mat m, Point p,int step)
{
	cout<<"unMark point=["<<p<<"]\n";
	rectangle(m,Point(p.x-step/2+2,p.y-step/2+2),Point(p.x+step/2-2, p.y+step/2-2),Scalar(255,255,255),CV_FILLED );
}
// sprawdza czy byl
bool isMarked(Mat m, Point p){
	uchar b = m.data[m.channels()*(m.cols*p.y + p.x) + 0];
	cout<<"isMarked ["<<(b == 125)<<"]\n";
	 if(b == 125) return true;
	 else
		 return false;
}

// sprawdza czy bedac na danym polu jest poza labiryntem:
// tak - koniec zwraca true
// nie sprawdza czy mozna sie ruszyc w jakims kierunku
// nie - koniec false, tak - wywolaj rekurencyjnie samego siebie
bool solveLab(Mat m, Point p, int step,Rect r,int ix,Point* exitPoints)
{
	cout<<"\nsolveLab point=["<<p<<"]\n";
	
	
	if (isOutside(p,r)) return true;//jezeli jestem poza labiryntem to koniec
	if(isMarked(m,p)) return false; //jezeli tu by³em koniec
	mark(m,p,step); // oznacz pole na ktorym stoisz
	 
	exitPoints[ix]=p; //zapamietuje pozycje
		ix++;		
	for (int i=0;i<4;i++){
		direction dir = direction(i);
		if(!wallExist(m,p,step,dir))
		{
			cout << "goto "<<dir<<endl;
			if(solveLab(m,addPoint(m,p,step,dir),step,r,ix,exitPoints)) 
			{
				
				return true;
			}
		}
	}
	unMark(m,p,step);  // tu nie ma wyjscia, odznacz, skasuj punkt zwroc false
	exitPoints[ix]=Point(-1,-1); 
	
	return false;
}

int main( int argc, char** argv ) {
	double w,h,fps;
	bool continueCapture = true;
	cv::namedWindow( "src", CV_WINDOW_AUTOSIZE );
	cv::namedWindow( "Theseus", CV_WINDOW_AUTOSIZE );
	cv::namedWindow( "labMat", CV_WINDOW_AUTOSIZE );
	
	 cv::VideoCapture cap(0);

	if ( !cap.isOpened() ) return -1;
	w = cap.get( CV_CAP_PROP_FRAME_WIDTH ); //get the width of frames of the video
	h = cap.get( CV_CAP_PROP_FRAME_HEIGHT ); //get the height of frames of the video
	fps = cap.get( CV_CAP_PROP_FPS ); // ile mamy klatek na sekunde?	
	
	bool tesFree = false; //jesli Tezeusz bedzie wolny to true
	Rect tesRect, labRect;

	Mat src;
	Mat thr(src.rows,src.cols,CV_8UC1); // do okrelenia biggestBloba labiryntu
	Mat teseusMat(src.rows,src.cols,CV_8UC1); // w niej szukam Tezeusza - czerwony
	Mat labiryntMat(src.rows,src.cols,CV_8UC1); // w nim jest poszukiwane wyjcie

	while( true ) {
		
		if ( cap.read( src )  ) {

			//src = imread("lab3.jpg",CV_LOAD_IMAGE_COLOR); // wczytywanie z obrazka

			flip(src,src,1);

			// szukanie tezeusza - po kolorze
			blur(src,teseusMat,Size(2,2));
			cvtColor(teseusMat,teseusMat,CV_BGR2HSV);			
			//inRange(teseusMat,Scalar(100, 50, 50),Scalar(250, 240, 240),teseusMat); //120 179	

			inRange(teseusMat,Scalar(150, 50, 50),Scalar(200, 240, 240),teseusMat); //- dobry dla druknietego labiryntu wiat³o ja¿eniowka
			//	inRange(teseusMat,Scalar(0, 135, 135),Scalar(30, 255, 400),dstA);
			//inRange(teseusMat, Scalar(330, 135, 135), Scalar(360, 255, 400), dstB);
			//bitwise_or(dstA,dstB,teseusMat);
			tesRect = biggestContour(teseusMat); // prost. dooko³a Tezeusza

			// przygotowanie obrazu labiryntu
			blur(src, labiryntMat, Size(2,2));				
			cvtColor(labiryntMat,labiryntMat,CV_RGB2GRAY ); //konw. na szary
			threshold(labiryntMat, labiryntMat,100, 255,THRESH_BINARY); //100
			
			// prost dooko³a labiryntu
			cvtColor(src,thr,CV_BGR2GRAY); 
			threshold(thr, thr,25, 255,THRESH_BINARY);
			bitwise_not(thr, thr); // odwroc kolory
			labRect = biggestContour(thr);

			rectangle(src, labRect,  Scalar(0,255,0),1, 8,0); //rysuje dooko³a labiryntu i teszeusza prost
			rectangle(src, tesRect,  Scalar(0,255,0),1, 8,0); 

			int step = tesRect.width-1; // d³ugoæ kroku Tezeusza

			Point tes = Point(tesRect.x+tesRect.width/2,tesRect.y+tesRect.height/2); // punkt wyjciowy
			
			int ix=0; // do tablicy wsp. kroków

			

			// zacznij szukanie tylko jeli punkt jest wewnatrz labiryntu (kamerka czasem lapie rozne rzeczy)
			if(labRect.contains(tesRect.tl()) && labRect.contains(tesRect.br())){ 

			// wielkosc tablicy nie jest wieksza niz ilosc wszystkich pol na mapie
			int n = (labRect.width/tesRect.width)*(labRect.height/tesRect.height);
			
			Point* exitPoints=new Point[n]; // tablica wspolrzednych krokow
			for(int i=0;i<n;i++)exitPoints[i]=Point(-1,-1);

			tesFree=solveLab(labiryntMat,tes,step,labRect,ix,exitPoints);
			//wyp. wspolrzedne
			for(int i=0;i<n;i++){ 
				cout<<exitPoints[i];
				if(exitPoints[i+1].x!=-1 )
					drawLine(src,exitPoints[i],exitPoints[i+1]);

			}

			
			if(tesFree){
				
				imwrite("tesIsFree.jpg",src);
				imwrite("tesIsFreeLab.jpg",labiryntMat);
			}
			delete [] exitPoints;
			}

			imshow( "src", src );
			imshow( "labMat", labiryntMat );
			imshow( "Theseus", teseusMat );
		
		} else break;
		if( waitKey( 30 ) == 27 || tesFree) break; //----|---------------------
		//if( waitKey( 30 ) == 27) break;
	}

	if(tesFree){
	teseusMat=imread("tesIsFree.jpg",CV_LOAD_IMAGE_COLOR);
	imshow( "Theseus", teseusMat );
	waitKey();
	}

	return 0;
}

