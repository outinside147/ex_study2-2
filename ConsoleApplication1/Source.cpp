#define _CRT_SECURE_NO_WARNINGS

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <tesseract/strngs.h>

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;
using namespace std;
using namespace tesseract;


// 2Ç¬ÇÃBOXÇî‰ärÇ∑ÇÈä÷êî
bool  Date_equal(BOX* box1, BOX* box2){
	//if (box1 == NULL || box2 == NULL) return false;
	return box1->x == box2->x && box1->y == box2->y && box1->w == box2->w && box1->h == box2->h;
}

// boxÇ™boxesÇÃíÜÇ…Ç†ÇÍÇŒtrueÅAÇªÇ§Ç≈Ç»ÇØÇÍÇŒfalseÇï‘Ç∑
bool set_member(BOX* box, Boxa* boxes){
	int i = 0;
	BOX* include_box = boxaGetBox(boxes, 0, L_CLONE);
	while (include_box != box && i != boxes->n - 1){
		i++;
		include_box = boxaGetBox(boxes, i, L_CLONE);
	}

	if (include_box == box){
		return true;
	}
	else {
		return false;
	}
}

void set_difference(Boxa* a_boxes, Boxa* b_boxes, Boxa* c_boxes){
	int i, j;
	for (i = 0, j = 0; i < a_boxes->n; i++){
		BOX* include_abox = boxaGetBox(a_boxes, i, L_CLONE);
		if (!set_member(include_abox, b_boxes)){
			boxaAddBox(c_boxes, include_abox, L_CLONE);
		}
	}
}


int main()
{
	int rparam_end = 893;
	int break_flg = 0;
	int nearest_y = 999;

	Boxa* suit_Wboxes = boxaCreate(6);
	BOX* box0 = boxCreate(119, 65, 9, 6);		// 1çsñ⁄
	BOX* box1 = boxCreate(363, 38, 37, 19);		// 
	BOX* box2 = boxCreate(410, 29, 110, 25);	// 
	BOX* box3 = boxCreate(529, 28, 74, 21);		// 
	BOX* box4 = boxCreate(102, 144, 65, 30);	// 2çsñ⁄
	BOX* box5 = boxCreate(176, 142, 32, 23);	//

	BOX* min_box = boxCreate(999, 999, 0, 0);

	Boxa* correct_boxes = boxaCreate(50);

	boxaAddBox(suit_Wboxes, box0, L_CLONE);
	boxaAddBox(suit_Wboxes, box1, L_CLONE);
	boxaAddBox(suit_Wboxes, box2, L_CLONE);
	boxaAddBox(suit_Wboxes, box3, L_CLONE);
	boxaAddBox(suit_Wboxes, box4, L_CLONE);
	boxaAddBox(suit_Wboxes, box5, L_CLONE);

	Boxa* test_boxes = boxaCreate(50);
	boxaAddBox(test_boxes, box0, L_CLONE);
	boxaAddBox(test_boxes, box1, L_CLONE);
	boxaAddBox(test_boxes, box2, L_CLONE);

	Boxa* dif_boxes = boxaCreate(50);

	set_difference(suit_Wboxes,test_boxes,dif_boxes);

	for (int i = 0; i < dif_boxes->n; i++) {
		BOX* box = boxaGetBox(dif_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}

}