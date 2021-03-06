#include <pthread.h>
#include <FreeImage.h>
#include <dirent.h>
#include <iostream>
#include <string.h>
#include <cmath>
#include <cassert>
#include <queue>
#include <mutex>

struct thread_data
{
	std::queue<std::string> *input;
	std::mutex *lock;
	double brightness;
	double gamma;
};

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

void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
	int rc = pthread_create(thread, attr, start_routine, arg);
	if (rc)
	{
		std::cerr << "Could not create thread." << std::endl;
		exit(1);
	}
}

bool process_image(std::string infile, std::string outfile, double brightness, double gamma)
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
	closedir(editdir);
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
			pixel.rgbRed = int(clamp(0, pow(((double) pixel.rgbRed) / 255.0f, gamma) * 255.0, 255));
			pixel.rgbBlue = int(clamp(0, pow(((double) pixel.rgbBlue) / 255.0f, gamma) * 255.0, 255));
			pixel.rgbGreen = int(clamp(0, pow(((double) pixel.rgbGreen) / 255.0f, gamma) * 255.0, 255));
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

void *process_thread(void *data)
{
	struct thread_data *args;
	args = (struct thread_data*) data;
	std::queue<std::string> *input = args -> input;
	std::mutex *lock = args -> lock;
	double brightness = args -> brightness;
	double gamma = args -> gamma;
	std::string infile;
	std::string outfile;
	bool exists;
	while (true)
	{
		exists = false;
		while (!(lock -> try_lock()));
		if (input -> empty())
		{
			lock -> unlock();
			break;
		}
		infile = input -> front();
		input -> pop();
		lock -> unlock();
		gen_edit_filename(infile, outfile);
		DIR *editdir = opendir("img/edit");
		struct dirent *file;
		while ((file = readdir(editdir)) != NULL)
		{
			if (strcmp(file -> d_name, outfile.c_str()) == 0)
			{
				exists = true;
				break;
			}
		}
		closedir(editdir);
		infile = "img/" + infile;
		outfile = "img/edit/" + outfile;
		if (exists) { continue; }
		else
		{
			while (!(lock -> try_lock()));
			std::cout << "Processing " << infile << std::endl;
			lock -> unlock();
		}
		FIBITMAP *input = FreeImage_Load(FIF_PNG, infile.c_str());
		if (input == NULL)
		{
			while (!(lock -> try_lock()));
			std::cerr << "Failed to load " << infile << std::endl;
			lock -> unlock();
			exit(1);
		}
		if (!FreeImage_Save(FIF_PNG, input, outfile.c_str()))
		{
			FreeImage_Unload(input);
			while (!(lock -> try_lock()));
			std::cerr << "Failed to save " << outfile << std::endl;
			lock -> unlock();
			exit(1);
		}
		FreeImage_Unload(input);
		FIBITMAP *output = FreeImage_Load(FIF_PNG, outfile.c_str());
		if (output == NULL)
		{
			while (!(lock -> try_lock()));
			std::cerr << "Failed to load " << outfile << std::endl;
			lock -> unlock();
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
				pixel.rgbRed = int(clamp(0, pow(((double) pixel.rgbRed) / 255.0f, gamma) * 255.0, 255));
				pixel.rgbBlue = int(clamp(0, pow(((double) pixel.rgbBlue) / 255.0f, gamma) * 255.0, 255));
				pixel.rgbGreen = int(clamp(0, pow(((double) pixel.rgbGreen) / 255.0f, gamma) * 255.0, 255));
				FreeImage_SetPixelColor(output, x, y, &pixel);
			}
		}
		if (!FreeImage_Save(FIF_PNG, output, outfile.c_str()))
		{
			FreeImage_Unload(input);
			while (!(lock -> try_lock()));
			std::cerr << "Failed to save " << outfile << std::endl;
			lock -> unlock();
			exit(1);
		}
		FreeImage_Unload(output);
		while (!(lock -> try_lock()));
		std::cout << "Saved " << outfile << std::endl;
		lock -> unlock();
	}
	pthread_exit(NULL);
}

void process_threaded(std::queue<std::string> *input, std::mutex *lock, double brightness, double gamma, unsigned int num_threads)
{
	pthread_t *threads = new pthread_t[num_threads];
	struct thread_data *td = new thread_data[num_threads];
	for (unsigned int i = 0; i < num_threads; i++)
	{
		td[i].input = input;
		td[i].lock = lock;
		td[i].brightness = brightness;
		td[i].gamma = gamma;
		create_thread(&threads[i], NULL, process_thread, (void*) &td[i]);
	}
	for (unsigned int i = 0; i < num_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}
	delete[] threads;
	delete[] td;
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
	closedir(editdir);
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " [brightness] <brightness> [gamma] <gamma>" << std::endl;
		exit(1);
	}
	double brightness = 1.0;
	double gamma = 1.0;
	unsigned int threads = 1;
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
		if (strcmp("gamma", argv[i]) == 0)
		{
			gamma = strtod(argv[i + 1], NULL);
			assert(gamma > 0);
		}
		if (strcmp("threads", argv[i]) == 0)
		{
			threads = strtoul(argv[i + 1], NULL, 0);
			assert(threads > 0);
		}
	}
	FreeImage_Initialise();
	std::string temp;
	if (threads == 1)
	{
		while ((file = readdir(imgdir)) != NULL)
		{
			if (ispng(file -> d_name))
			{
				gen_edit_filename(file -> d_name, temp);
				if (!ispng(temp))
				{
					std::cout << "Processing " << file -> d_name << std::endl;
					gen_edit_filename(file -> d_name, outfile);
					if (process_image(file -> d_name, outfile.c_str(), brightness, gamma))
					{
						std::cout << "Saved " << outfile << std::endl;
					}
				}
			}
		}
	}
	else
	{
		std::queue<std::string> input;
		std::mutex lock;
		while ((file = readdir(imgdir)) != NULL)
		{
			if (ispng(file -> d_name))
			{
				input.push(file -> d_name);
			}
		}
		process_threaded(&input, &lock, brightness, gamma, threads);
	}
	closedir(imgdir);
	FreeImage_DeInitialise();
	return 0;
}
