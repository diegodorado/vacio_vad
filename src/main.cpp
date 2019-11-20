#include "ofMain.h"
#include "ofApp.h"
#include <stdio.h>

//========================================================================
int main(int argc, char *argv[]){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

/*
	std::cout << "name of program: " << argv[0] << '\n' ;

	if( argc > 1 )
	{
			std::cout << "there are " << argc-1 << " (more) arguments, they are:\n" ;

			std::copy( argv+1, argv+argc, std::ostream_iterator<const char*>( std::cout, "\n" ) ) ;
	}
*/

	ofApp *app = new ofApp();

	app->arguments = vector<string>(argv, argv + argc);

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(app); // start the app

}
