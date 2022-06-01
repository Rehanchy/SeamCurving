//   Copyright © 2021, Renjie Chen @ USTC

#pragma once

#include <cassert>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <math.h>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)


using namespace std;

int min(int a, int b)
{
	if (a < b)
		return 0;
	else
		return 1;
}

int min(int a, int b, int c) { // compare and return
	if (a < b) 
	{
		if (a < c) 
			return -1;
		else
			return 1;
	}
	else 
	{
		if (b < c) 
			return 0;
		else 
			return 1;
	}
}

vector<vector<int> > Transpose(vector< vector<int> > data)
{
	vector<vector<int> > result(data[0].size());

	for (int i = 0; i < data.size(); i++) {
		for (int j = 0; j < data[i].size(); j++) {
			result[j].push_back(data[i][j]);
		}
	}
	return result;
}

class MyImage
{
private:
	std::vector<BYTE> pixels;
	int w, h, comp;

public:
	MyImage() :w(0), h(0), comp(0) {}
	~MyImage() { }

	MyImage(const std::string& filename, int ncomp = 4) :w(0), h(0), comp(0)
	{
		stbi_set_flip_vertically_on_load(true);
		BYTE* data = stbi_load(filename.data(), &w, &h, &comp, ncomp);

		if (data) {
			pixels = std::vector<BYTE>(data, data + w * h * comp);
			stbi_image_free(data);
		}
		else {
			fprintf(stderr, "failed to load image file %s!\n", filename.c_str());
			struct stat info;
			if (stat(filename.c_str(), &info))
				fprintf(stderr, "file doesn't exist!\n");
		}
	}


	MyImage(BYTE* data, int ww, int hh, int pitch, int ncomp = 3) :w(ww), h(hh), comp(ncomp)
	{
		if (pitch == w * comp) pixels = std::vector<BYTE>(data, data + pitch * h);
		else {
			pixels.resize(w * comp * h);
			for (int i = 0; i < h; i++) std::copy_n(data + pitch * i, pitch, pixels.data() + i * w * comp);
		}
	}

	static int alignment() { return 1; }  // OpenGL only supports 1,2,4,8, do not use 8, it is buggy

	inline bool empty() const { return pixels.empty(); }

	inline BYTE* data() { return pixels.data(); }
	inline const BYTE* data() const { return pixels.data(); }
	inline int width() const { return w; }
	inline int height() const { return h; }
	inline int dim() const { return comp; }
	inline int pitch() const { return w * comp; }

	MyImage rescale(int ww, int hh) const
	{
		std::vector<BYTE> data(ww * comp * hh);
		stbir_resize_uint8(pixels.data(), w, h, w * comp,
			data.data(), ww, hh, ww * comp, comp);

		return MyImage(data.data(), ww, hh, ww * comp, comp);
	}


	MyImage resizeCanvas(int ww, int hh)
	{
		std::vector<BYTE> data(ww * comp * hh, 255);
		for (int i = 0; i < h; i++)
			std::copy_n(pixels.data() + i * w * comp, w * comp, data.data() + i * ww * comp);

		return MyImage(data.data(), ww, hh, ww * comp, comp);
	}



	inline void write(const std::string& filename, bool vflip = true) const {
		if (filename.size() < 4 || !_strcmpi(filename.data() + filename.size() - 4, ".png")) {
			stbi_write_png(filename.data(), w, h, comp, pixels.data() + (vflip ? w * comp * (h - 1) : 0), w * comp * (vflip ? -1 : 1));
		}
		else {
			fprintf(stderr, "only png file format(%s) is supported for writing!\n", filename.c_str());
		}
	}

	inline std::vector<BYTE> bits(int align = 1) const
	{
		const int pitch = (w * comp + align - 1) / align * align;

		std::vector<BYTE> data(pitch * h);
		for (int i = 0; i < h; i++)
			std::copy_n(pixels.data() + i * w * comp, w * comp, data.data() + i * pitch);

		return data;
	}
	// Modified
	// get pixel data by coordinate
	vector<BYTE> getpix(int i, int j) const
	{
		vector<BYTE> pix_data(comp);
		copy_n(pixels.data() + comp * (i * w + j), comp, pix_data.data());
		return pix_data;
	}

	// change pixel data by coordinate
	void setpix(int i, int j, vector<BYTE> nwData)
	{
		copy_n(nwData.data(), comp, pixels.data() + comp * (i * w + j));
	}

	// get grey pic
	MyImage Get_Grey() const
	{
		vector<BYTE> Grey(w * h);
		int temp;
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				temp = 0;
				for (int k = 0; k < 3; k++)
				{
					temp += this->pixels[(i * w + j) * comp + k];
				}
				temp /= 3;
				Grey[i * w + j] = (BYTE)temp;
			}
		}
		return MyImage(Grey.data(), w, h, w, 1);
	}

	// delete by row given the array which specifies the coordinate
	MyImage Row_Delete(vector<int> index) const
	{
		vector<BYTE> NewImg(comp * w * (h - 1));
		for (int j = 0; j < w; j++)
		{
			int i = 0;
			for (; i < index[j]; i++)
				copy_n(pixels.data() + comp * (i * w + j), comp, NewImg.data() + comp * (i * w + j));
			// index[j], j is gone
			for (; i < h - 1; i++)
				copy_n(pixels.data() +  comp * ((i + 1) * w + j), comp, NewImg.data() + comp * (i * w + j));
		}
		return MyImage(NewImg.data(), w, h - 1, comp * w, comp);
	}

	// same as delete by row, this time column
	MyImage Col_Delete(vector<int> index) const
	{
		vector<BYTE> NewImg(comp * (w - 1) * h);
		for (int i = 0; i < h; i++)
		{
			copy_n(pixels.data() + (i * w) * comp, index[i] * comp, NewImg.data() + (i * (w - 1)) * comp);
			// i, index[i] is gone
			copy_n(pixels.data() + (i * w + index[i] + 1) * comp, (w - index[i] - 1) * comp, NewImg.data() + (i * (w - 1) + index[i]) * comp);
		}
		return MyImage(NewImg.data(), w - 1, h, (w - 1) * comp, comp);
	}

	// add by row, for the need of better performance of pic expansion, we may add_n_row a time
	MyImage Row_Add(vector<int> index) const
	{
		vector<BYTE> NewImg(comp * w * (h + 1));
		int c = 0, r1 = 0, r2 = 0;
		int i = 0;
		while (c < w)
		{
			r1 = 0;
			r2 = 0;
			while (r1 < h && r2 < h + 1)
			{
				if (r1 == index[i])
				{
					for (int k = 0; k < 3; k++)
						NewImg[comp * (r2 * w + c) + k] = pixels[comp * (r1 * w + c) + k];
					r2++;
				}
				for (int k = 0; k < 3; k++)
					NewImg[comp * (r2 * w + c) + k] = pixels[comp * (r1 * w + c) + k];
				r1++; r2++;
			}
			c++;
			i++;
		}
		return MyImage(NewImg.data(), w, h + 1, w  * comp, comp);
	}

	MyImage Row_Add_for_Energy(vector<int> index) const
	{
		vector<BYTE> NewImg(comp * w * (h + 1));
		int c = 0, r1 = 0, r2 = 0;
		int i = 0;
		while (c < w)
		{
			r1 = 0;
			r2 = 0;
			while (r1 < h && r2 < h + 1)
			{
				if (r1 == index[i])
				{
					for (int l = 0; l < 2; l++) // copy twice and reset the energy value higher, so that it won't be choosed at least next time
					{
						for (int k = 0; k < 3; k++)
							NewImg[comp * (r2 * w + c) + k] = pixels[comp * (r1 * w + c) + k] + 15;
						if (comp == 4)
							NewImg[comp * (r2 * w + c) + 3] = pixels[comp * (r1 * w + c) + 3];
						r2++;
					}
					r1++;
				}
				else
				{
					for (int k = 0; k < 3; k++)
						NewImg[comp * (r2 * w + c) + k] = pixels[comp * (r1 * w + c) + k];
					r1++; r2++;
				}
			}
			c++;
			i++;
		}
		return MyImage(NewImg.data(), w, h + 1, w * comp, comp);
	}

	// add by column, for the need of better performance of pic expansion, we shall add_n_column a time
	MyImage Col_Add(vector<int> index) const
	{
		vector<BYTE> NewImg(comp * (w + 1) * h);
		int r = 0, c1 = 0, c2 = 0;
		int i = 0;
		while (r < h)
		{
			c1 = 0;
			c2 = 0;
			while (c1 < w && c2 < w + 1)
			{
				if (c1 == index[i])
				{
					for (int k = 0; k < 3; k++)
						NewImg[comp * (r * (w + 1) + c2) + k] = pixels[comp * (r * w + c1) + k];
					c2++;
				}
				for (int k = 0; k < 3; k++)
					NewImg[comp * (r * (w + 1) + c2) + k] = pixels[comp * (r * w + c1) + k];
				c1++; c2++;
			}
			r++;
			i++;
		}
		return MyImage(NewImg.data(), w + 1, h, (w + 1) * comp, comp);
	}

	MyImage Col_Add_for_Energy(vector<int> index) const
	{
		vector<BYTE> NewImg(comp * (w + 1) * h);
		int r = 0, c1 = 0, c2 = 0;
		int i = 0;
		while (r < h)
		{
			c1 = 0;
			c2 = 0;
			while (c1 < w && c2 < w + 1)
			{
				if (c1 == index[i])
				{
					for (int l = 0; l < 2; l++) // copy twice and reset the energy value higher, so that it won't be choosed at least next time
					{
						for (int k = 0; k < 3; k++)
							NewImg[comp * (r * (w + 1) + c2) + k] = pixels[comp * (r * w + c1) + k] + 15;
						if (comp == 4)
							NewImg[comp * (r * (w + 1) + c2) + 3] = pixels[comp * (r * w + c1) + 3];
						c2++;
					}
					c1++;
				}
				else
				{
					for (int k = 0; k < 3; k++)
						NewImg[comp * (r * (w + 1) + c2) + k] = pixels[comp * (r * w + c1) + k];
					c1++; c2++;
				}
			}
			r++;
			i++;
		}
		return MyImage(NewImg.data(), w + 1, h, (w + 1) * comp, comp);
	}

	vector<int> Energy_Cost_Row(MyImage input) const	 
	{
		vector<int> cost(w * h);
		int down, left, up;
		int index;
		for (int i = 0; i < h; i++) 
			cost[i * w] = input.pixels[i * w]; // same value as Image read
		for (int j = 1; j < w; j++) 
		{
			index = min(cost[j - 1], cost[w + j - 1]); // if j-1 < w+j-1 return 0 this is the smallest
			cost[j] = cost[index * w + j - 1] + input.pixels[j]; // fill first row
			for (int i = 1; i < h - 1; i++)
			{
				up = cost[(i - 1) * w + j - 1];		// rightup
				left = cost[i * w + j - 1];			// right
				down = cost[(i + 1) * w + j - 1];		//rightdown
				index = min(up, left, down);

				cost[i * w + j] = cost[(i + index) * w + j - 1] + input.pixels[i * w + j]; // fill j th col
			}
			index = min(cost[(h - 2) * w + j - 1], cost[(h - 1) * w + j - 1]);
			cost[(h - 1) * w + j] = cost[(h - 2 + index) * w + j - 1] + input.pixels[(h - 1) * w + j];
		}
		return cost; 
		// after calculating vector<int> cost(w * h) has stored the information about
		// the least cost route, you can get it from sorting the w th colmun, which means rightest one, and choose the smallest
		// how the route formed, cost(i, j) - input.pixels(i, j), compare it with the three cost value in j-1 th colmun, 
		// when you get a 'equal', that means you get the way
		// the implementation is just in the next function
	}

	vector<int> Seamindex_Row(MyImage input) const		// get seam for row
	{
		vector<int> cost = this->Energy_Cost_Row(input);
		vector<int> index(w);

		int currRow = 0;
		for (int i = 1; i < h; i++) 
		{
			if (cost[(i + 1) * w - 1] < cost[(currRow + 1) * w - 1])  // from the rightest to calculate
				currRow = i;
		}
		index[w - 1] = currRow;
		int pathFinder = cost[(currRow + 1) * w - 1] - input.pixels[(currRow + 1) * w - 1];
		for (int j = w - 2; j >= 0; pathFinder = cost[currRow * w + j] - input.pixels[currRow * w + j], j--)
		{
			if (currRow == 0) // if it is the first row
			{
				if (pathFinder == cost[j])
				{
					index[j] = currRow;		//left
				}
				else
				{
					currRow += 1;		//down left
					index[j] = currRow;
				}
				continue;
			}
			if (pathFinder == cost[currRow * w + j])
			{
				index[j] = currRow;		//left
				continue;
			}
			else if (pathFinder == cost[(currRow - 1) * w + j])
			{
				currRow -= 1;		//up left
				index[j] = currRow;
			}
			else 
			{
				currRow += 1;		//down left
				index[j] = currRow;
			}					
		}
		return index;
	}

	vector<vector<int>> nSeamindex_Row(MyImage input, int n) const // it is for the expansion of the pciture
	{
		vector<int> cost = this->Energy_Cost_Row(input);
		vector<vector<int>> index(w); // which shall be returned
		vector<int> lastrow(h);
		for (int i = 0; i < h; i++)
			lastrow[i] = cost[i * w + w - 1];

		sort(lastrow.begin(), lastrow.end()); // this time we need multiple start point

		int repeat = 0, count = 0;
		for (int k = 0; k < n; k++)  // find n paths
		{
			if (k > 0 && lastrow[k - 1] == lastrow[k]) 
				repeat += 1;
			else
				repeat = 0;
			count = repeat;
			int currRow = 0;
			for (int i = 0; i < h; i++) 
			{
				if (lastrow[k] == cost[i * w + w - 1])
				{
					count--;
					if (count < 0) 
					{
						currRow = i;
						break;
					}
				}
			}

			index[w - 1].push_back(currRow);
			int pathFinder = cost[(currRow + 1) * w - 1] - input.pixels[(currRow + 1) * w - 1];
			for (int j = w - 2; j >= 0; pathFinder = cost[currRow * w + j] - input.pixels[currRow * w + j],  j--)
			{
				if (currRow == 0) {
					if (pathFinder == cost[j]) 
						index[j].push_back(currRow);		//left
					else 
					{
						currRow += 1;		//down left
						index[j].push_back(currRow);
					}
					continue;
				}
				if (pathFinder == cost[currRow * w + j]) 
				{
					index[j].push_back(currRow);		//left
					continue;
				}
				else if (pathFinder == cost[(currRow - 1) * w + j])
				{
						currRow -= 1;		//up left
						index[j].push_back(currRow);
				}
				else 
				{
					currRow += 1;		//down left
					index[j].push_back(currRow);
				}
			}
		}
		return index;
	}

	// it is just the same with row
	vector<int>  Energy_Cost_Col(MyImage input) const
	{
		vector<int> cost(w * h);
		int left, right, up;
		int index;
		for (int j = 0; j < w; j++)		
			cost[j] = input.pixels[j];
		for (int i = 1; i < h; i++) 
		{
			index = min(cost[(i - 1) * w], cost[(i - 1) * w + 1]);
			cost[i * w] = cost[(i - 1) * w + index] + input.pixels[i * w];
			for (int j = 1; j < w - 1; j++) 
			{
				left = cost[(i - 1) * w + j - 1];		//up left
				up = cost[(i - 1) * w + j];
				right = cost[(i - 1) * w + j + 1];		//up right
				index = min(left, up, right);

				cost[i * w + j] = cost[(i - 1) * w + j + index] + input.pixels[i * w + j];
			}
			index = min(cost[i * w - 1], cost[i * w]);
			cost[i * w + w - 1] = cost[i * w - 1 + index - 1] + input.pixels[i * w + w - 1];

		}
		return cost;
	}

	vector<int> Seamindex_Col(MyImage input) const
	{
		vector<int> cost = this->Energy_Cost_Col(input);
		vector<int> index(h);
		int currCol = 0;
		for (int j = 1; j < w; j++) 
			if (cost[(h - 1) * w + j] < cost[(h - 1) * w + currCol])
				currCol = j;
		index[h - 1] = currCol;
		int pathFinder = cost[(h - 1) * w + currCol] - input.pixels[(h - 1) * w + currCol];
		for (int i = h - 2; i >= 0; pathFinder = cost[i * w + currCol] - input.pixels[i * w + currCol], i--)
		{
			if (currCol == 0) // if it is the fisrt column
			{
				if (pathFinder == cost[i * w + currCol])
				{
					index[i] = currCol;		//up
				}
				else 
				{
					currCol += 1;		//up right
					index[i] = currCol;
				}
				continue;
			}
			if (pathFinder == cost[i * w + currCol - 1])
			{
				currCol -= 1;		//up left
				index[i] = currCol;
				continue;
			}
			else if (pathFinder == cost[i * w + currCol])
				index[i] = currCol;		//up
			else 
			{
				currCol += 1;		//up right
				index[i] = currCol;
			}					
		}
		return index;
	}

	double color_distance(MyImage input, int row1, int col1, int row2, int col2) 
	{
		auto c1 = input.getpix(row1, col1);
		auto c2 = input.getpix(row2, col2);
		double dc0 = ((double)c1[0] - (double)c2[0]) / 255, dc1 = ((double)c1[1] - (double)c2[1]) / 255, dc2 = ((double)c1[2] - (double)c2[2]) / 255;
		return sqrt(dc0 * dc0 + dc1 * dc1 + dc2 * dc2);
	}

	double distance(MyImage input, int row1, int col1, int row2, int col2) 
	{
		double color_dis = color_distance(input, row1, col1, row2, col2);
		double dRow = (row1 - row2 + 0.0) / input.h, dCol = (col1 - col2 + 0.0) / input.w;
		double xy_dis = sqrt(dRow * dRow + dCol * dCol);
		return color_dis / (1 + 3 * xy_dis); // C = 3
	}
	double salient(MyImage input, int r, int c) 
	{
		int rows = input.height();
		int cols = input.width();
		double diffs = 0.0;
		int count = 0;
		/*for (int row = 0; row <= rows; ++row)
		{
			for (int col = 0; col <= cols; ++col)
			{
				diffs.push_back(distance(input, r, c, row, col));
				count++;
			}
		}*/ // when full scale CA calculation use this
		for (int row = r - 2; row <= r + 2; ++row)
		{
			if (row < 0 || row >= rows) 
				continue;
			for (int col = c - 2; col <= c + 2; ++col)
			{
				if (col < 0 || col >= cols) 
					continue;
				diffs+=(distance(input, r, c, row, col));
				count++;
			}
		}
		//sort(diffs.begin(), diffs.end());
		//double sum = 0;
		//int n = 0;
		//for (n = 0; n <= 64 && n < diffs.size(); ++n) // k = 64
			//sum += diffs[n];
		return 1 - exp(-diffs / count);
	}

	MyImage Get_Src(MyImage input, int u) // mix the pixel of the origin picture
	{
		int cols = input.w;
		int rows = input.h;
		int comp = input.comp;
		vector<BYTE> src(cols * rows * comp);
		for (int row = 0; row < rows; ++row)
		{
			for (int col = 0; col < cols; ++col)
			{
				int n = 0;
				int l = 0, a = 0, b = 0;
				for (int r = row - u; r <= row + u; ++r)
				{
					if (r < 0 || r >= rows) continue;
					for (int c = col - u; c <= col + u; ++c)
					{
						if (c < 0 || c >= cols) continue;
						++n;
						l += input.getpix(r, c)[0];
						a += input.getpix(r, c)[1];
						b += input.getpix(r, c)[2];
					}
				}
				src[comp * (row * cols + col)] = (BYTE)(l / n);
				src[comp * (row * cols + col) + 1] = (BYTE)(a / n);
				src[comp * (row * cols + col) + 2] = (BYTE)(b / n);
			}
		}
		return MyImage(src.data(), cols, rows, cols * comp, comp);
	}

	vector<double> saliencyMatrix(MyImage input, int u) // calculate the Saliency for each pixel
	{
		int cols = input.w;
		int rows = input.h;
		int comp = input.comp;
		vector<double> tg(cols * rows);
		vector<double> mid(cols * rows);
		MyImage src = Get_Src(input, u);
		cout << "finished Src\n";
		double _max = 0;
		for (int row = 0; row < rows; ++row) 
		{
			for (int col = 0; col < cols; ++col) 
			{
				double value = salient(src, row, col);
				mid[row*cols+col] = value;
				cout << "at " << row << col << " " << endl;
				if (value > _max) _max = value;
			}
		}
		for (int row = 0; row < rows; ++row) 
		{
			for (int col = 0; col < cols; ++col) 
			{
				double value = mid[row * cols + col] / _max;
				tg[row * cols + col] = value * 255;
			}
		}
		return tg;
	}

	MyImage Context_Aware_Saliency(MyImage input) // get final Saliency value
	{
		vector<double> rs1,rs2,rs3;
		int cols = input.w;
		int rows = input.h;
		vector<BYTE> final(cols * rows * comp);
		rs1 = saliencyMatrix(input, 2);
		cout << "finished";
		rs2 = saliencyMatrix(input, 1);
		cout << "finished";
		rs3 = saliencyMatrix(input, 0);
		for (int row = 0; row < rows; ++row)
		{
			for (int col = 0; col < cols; ++col)
			{
				auto avg = (int)(rs1[row * cols + col] + rs2[row * cols + col] + rs3[row * cols + col]) / 3;
				for (int k = 0; k < 3; k++)
					final[comp * (row * cols + col) + k] = (BYTE)avg;
			}
		}
		return MyImage(final.data(), cols, rows, cols * comp, comp);
	}
};
