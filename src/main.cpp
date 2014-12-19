#include <pthread.h>
#include <FreeImage.h>
#include <dirent.h>
#include <iostream>
#include <string.h>

double clamp(double a, double x, double b)
{
	if (x < a) { return a; }
	if (x > b) { return b; }
	return x;
}

bool ispng(std::string filename)
{
	return filename.find(".png") != std::string::npos;
}

void gen_edit_filename(std::string infile, std::string &outfile)
{
	outfile = infile.substr(0, infile.length() - 4) + "E.png";
}

bool process_image(std::string infile, std::string outfile, double brightness, double blur)
{
	DIR *editdir = opendir("img/edit");
	struct dirent *file;
	while ((file = readdir(editdir)) != NULL)
	{
		if (strcmp(file -> d_name, outfile.c_str()) == 0)
		{
			return false;
		}
	}
	infile = "img/" + infile;
	outfile = "img/edit/" + outfile;
	FIBITMAP *input = FreeImage_Load(FIF_PNG, infile.c_str());
	if (input == NULL)
	{
		std::cerr << "Failed to load " << infile << std::endl;
		exit(1);
	}
	if (!FreeImage_Save(FIF_PNG, input, outfile.c_str()))
	{
		FreeImage_Unload(input);
		std::cerr << "Failed to save " << outfile << std::endl;
		exit(1);
	}
	FreeImage_Unload(input);
	FIBITMAP *output = FreeImage_Load(FIF_PNG, outfile.c_str());
	if (output == NULL)
	{
		std::cerr << "Failed to load " << outfile << std::endl;
		exit(1);
	}
	unsigned int img_w = FreeImage_GetWidth(output);
	unsigned int img_h = FreeImage_GetHeight(output);
	RGBQUAD pixel;
	for (unsigned int x = 0; x < img_w; x++)
	{
		for (unsigned int y = 0; y < img_h; y++)
		{
			FreeImage_GetPixelColor(output, x, y, &pixel);
			pixel.rgbRed = int(clamp(0, pixel.rgbRed * brightness, 255));
			pixel.rgbBlue = int(clamp(0, pixel.rgbBlue * brightness, 255));
			pixel.rgbGreen = int(clamp(0, pixel.rgbGreen * brightness, 255));
			FreeImage_SetPixelColor(output, x, y, &pixel);
		}
	}
	if (!FreeImage_Save(FIF_PNG, output, outfile.c_str()))
	{
		FreeImage_Unload(input);
		std::cerr << "Failed to save " << outfile << std::endl;
		exit(1);
	}
		FreeImage_Unload(output);
	return true;
}

int main(int argc, char **argv)
{
	DIR *imgdir = opendir("img/");
	DIR *editdir = opendir("img/edit");
	struct dirent *file;
	std::string outfile;
	if (imgdir == NULL)
	{
		std::cerr << "Can't open ./img." << std::endl;
		exit(1);
	}
	if (editdir == NULL)
	{
		std::cerr << "Can't open ./img/edit." << std::endl;
		exit(1);
	}
	double brightness = 1;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp("brightness", argv[i]) == 0)
		{
			if (argc > i + 1)
			{
				brightness = strtod(argv[i + 1], NULL);
				assert(brightness > 0);
			}
		}
	}
	while ((file = readdir(imgdir)) != NULL)
	{
		if (ispng(file -> d_name))
		{
			std::cout << "Processing " << file -> d_name << std::endl;
			gen_edit_filename(file -> d_name, outfile);
			if (process_image(file -> d_name, outfile.c_str(), brightness, 0))
			{
				std::cout << "Saved " << outfile << std::endl;
			}
		}
	}
	return 0;
}
