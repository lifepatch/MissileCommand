#include "testApp.h"


void testApp::setup(){

	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(0);
	ofEnableAlphaBlending();

	ofDisableArbTex();

	startup.loadSound("start.wav");
	launch.loadSound("launch.wav");
	explode.loadSound("explode.wav");
	city.loadImage("city.png");
	silo.loadImage("silo.png");
	bg.loadImage("bg.png");

	int numCities = NUM_CITIES;
	for(int i = 0; i < numCities; i++){
		City o;
		o.setup(&city);
		o.color = ofColor(30,30,100);
		o.position.y = ofGetHeight() - city.height * 1.5;
		o.position.x = ofGetWidth() * 0.15 + ofGetWidth() * 0.7 * i / (numCities - 1) - city.width * 0.5;
		cities.push_back(o);
	}

	Silo *s = new Silo();
	s->setup(&silo);
	s->color = ofColor(30,30,100);
	s->position = SILO_LEFT - ofVec2f(silo.getWidth() * 0.5, silo.getHeight() * 0.5);
	s->setMissileSpeed(GOOD_MISSILE_SPEED);
	silos.push_back(s);

	s = new Silo();
	s->setup(&silo);
	s->color = ofColor(30,30,100);
	s->setMissileSpeed(GOOD_MISSILE_SPEED * 2);
	s->position = SILO_MID - ofVec2f(silo.getWidth() * 0.5, silo.getHeight() * 0.5);
	silos.push_back(s);

	s = new Silo();
	s->setup(&silo);
	s->setMissileSpeed(GOOD_MISSILE_SPEED);
	s->color = ofColor(30,30,100);
	s->position = SILO_RIGHT - ofVec2f(silo.getWidth() * 0.5, silo.getHeight() * 0.5);
	silos.push_back(s);


	// explosions

	MEDIA_LOAD_TEXTURE("smoke2.png", TEX_SMOKE);
	MEDIA_GET_TEXURE(TEX_SMOKE)->setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

	MEDIA_LOAD_TEXTURE("fire.png", TEX_FIRE);
	MEDIA_GET_TEXURE(TEX_FIRE)->setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

	MEDIA_LOAD_TEXTURE("explosion.png", TEX_EXPLOSION);
	MEDIA_GET_TEXURE(TEX_EXPLOSION)->setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

	e.setup( MEDIA_GET_TEXURE(TEX_SMOKE), MEDIA_GET_TEXURE(TEX_FIRE), MEDIA_GET_TEXURE(TEX_EXPLOSION) );
	e.setSmokeProperties(
						 5,					//random spawn pos offset from debris
						 ofVec2f(0,-50),	//smoke particles speed in pix / sec
						 ofVec2f(150,0),		//smoke particles acc in pix / sec
						 0.9					//life in seconds
						 );

	strength = 400;
	numP = 10;
	friction = 0.94;
	life = 0.9;
	offset = 0;
	spawnOffset = 5;
	smokeLife = 0.9;
	gravity = 200;


	///////////
#ifdef USE_REMOTE_CAMERA
	remoteCam.initGrabber(640,480);
	remoteCam.start();
	remoteCam.setRequestedImageType(OF_IMAGE_GRAYSCALE);
	remoteCam.setRequestedCompressionQuality( 90 );//jpeg, [0..100]
#endif

	laser.setup(ofRectangle(0,0, ofGetWidth(), ofGetHeight()), this, 640, 480, OF_IMAGE_GRAYSCALE);

}


void testApp::update(){

	float dt = 1/60.;


	e.setSmokeProperties(
						 spawnOffset,					//random spawn pos offset from debris
						 ofVec2f(0,-50),	//smoke particles speed in pix / sec
						 ofVec2f(50,0),	//smoke particles acc in pix / sec
						 smokeLife					//life in seconds
						 );

	e.update(1./60.);

	///////////////

#ifdef USE_REMOTE_CAMERA
	remoteCam.update();

	if(remoteCam.isFrameNew()){
		laser.update( remoteCam.getPixels() );
	}
#endif

	/////////////////



	switch (state) {

		case PLAYING:{

			//remove dead missiles
			for(int i = playerMissiles.size()-1; i >= 0 ; i--){
				playerMissiles[i]->update(dt);
				if (playerMissiles[i]->dead()){
					playerMissiles.erase(playerMissiles.begin() + i);
				}
			}
			for(int i = badMissiles.size() - 1; i >= 0; i--){
				badMissiles[i]->update(dt);
				if (badMissiles[i]->dead()){
					badMissiles.erase(badMissiles.begin() + i);
				}
			}

			//collide missiles with missiles
			//vector<int> badMissilesToDelete;
			for(int j = 0; j < playerMissiles.size(); j++){
				for(int i = 0; i < badMissiles.size(); i++){
					if ( playerMissiles[j]->pos.distance(badMissiles[i]->pos) < playerMissiles[j]->currentExplosionRadius){
						if (!badMissiles[i]->exploded){
							badMissiles[i]->explode();
							explode.play();
						}
						//if ( std::find(badMissilesToDelete.begin(), badMissilesToDelete.end(), i) == badMissilesToDelete.end() ){
							//badMissilesToDelete.push_back(i);
						//}
					}
				}
			}

			for(int j = 0; j < badMissiles.size(); j++){
				for(int i = j+1; i < badMissiles.size(); i++){
					if ( badMissiles[j]->exploded &&
						badMissiles[j]->pos.distance(badMissiles[i]->pos) < badMissiles[j]->currentExplosionRadius){
						if (!badMissiles[i]->exploded){
							badMissiles[i]->explode();
							explode.play();
						}
					}
				}
			}

			//remove collided missiles
//			for(int i = 0; i < badMissilesToDelete.size() ; i++){
//				delete badMissiles[ badMissilesToDelete[i] ];
//				badMissiles.erase(badMissiles.begin() + badMissilesToDelete[i]);
//			}

			//collide bad missiles with cities
			for(int i = badMissiles.size() - 1; i >= 0; i--){
				if (badMissiles[i]->exploded){
					for(int j = 0; j<cities.size(); j++){
						if ( cities[j].hitTestCircle(badMissiles[i]->pos, badMissiles[i]->explosionRadius()) ){
							if ( cities[j].isAlive() ){
								
								cities[j].kill();
								explode.play();
								e.explode(	cities[j].getCenter(),
										  strength,	//explosion strength
										  numP,		//num debris particles
										  friction,	//air friction
										  ofVec2f(0, gravity), // gravity vector
										  life,		//particles life duration
										  offset		//initial position randomness, should be exploded object radius +-
										  );
							}
						}
					}
					for(int j = 0; j<silos.size(); j++){
						if ( silos[j]->hitTestCircle(badMissiles[i]->pos, badMissiles[i]->explosionRadius()) ){
							if ( silos[j]->isAlive() ){

								silos[j]->kill();
								explode.play();
								e.explode(	silos[j]->getCenter(),
										  strength,	//explosion strength
										  numP,		//num debris particles
										  friction,	//air friction
										  ofVec2f(0, gravity), // gravity vector
										  life,		//particles life duration
										  offset		//initial position randomness, should be exploded object radius +-
										  );
							}
						}
					}
				}
			}

			}break;

		default:
			break;
	}
}




void testApp::draw(){

	//ofShowCursor();

	ofSetColor(255);
	bg.draw(0, 0, ofGetWidth(), ofGetHeight());

	ofSetColor(255, 128);
	laser.draw();

	
	for(int i = 0; i < cities.size(); i++){
		cities[i].draw();
	}
	for(int i = 0; i < silos.size(); i++){
		silos[i]->draw();
	}

	if (state == PLAYING){
		for(int i = 0; i < cities.size(); i++){
			cities[i].draw();
		}
		for(int i = 0; i < badMissiles.size(); i++){
			badMissiles[i]->draw();
		}
		for(int i = 0; i < playerMissiles.size(); i++){
			playerMissiles[i]->draw();
		}
	}

	e.drawMesh();

}


void testApp::startLevel(){

	startup.play();
	e.stopPostExplosionSmoke();
	state = PLAYING;
	levelTime = LEVEL_TIME;
	playerMissiles.clear();
	badMissiles.clear();
	
	for(int i = 0; i <NUM_BAD_MISSILES; i++){
		Missile *m = new Missile();
		m->startBad();
		badMissiles.push_back(m);
	}

	for(int i = 0; i < silos.size(); i++){
		silos[i]->revive();
		silos[i]->resetMissiles();
	}

	for(int i = 0; i < cities.size(); i++){
		cities[i].revive();
	}
}


Silo* testApp::bestSiloForTarget(ofVec2f target){

	vector<Silo*> validSilos;
	for(int i = 0; i < silos.size(); i++){
		if (silos[i]->canLaunchMissile()){
			validSilos.push_back(silos[i]);
		}
	}
	Silo * bestSilo = NULL;

	float minDist = 99999999.0; //FLOAT_MAX no thtere?
	for(int i = 0; i < validSilos.size(); i++){
		float d = validSilos[i]->getCenter().distance(target);
		if ( d < minDist){
			minDist = d;
			bestSilo = validSilos[i];
		}
	}
	return bestSilo;
}


void testApp::exit(){
	remoteCam.close();
}


void testApp::keyPressed(int key){

	if(key==' ')ofToggleFullscreen();
		
	startLevel();
}


void testApp::keyReleased(int key){

}


void testApp::mouseMoved(int x, int y){

}
void testApp::mouseDragged(int x, int y, int button){

}


void testApp::mousePressed(int x, int y, int button){

//	e.explode(	ofVec2f(x,y),
//			  strength,	//explosion strength
//			  numP,		//num debris particles
//			  friction,	//air friction
//			  ofVec2f(0, gravity), // gravity vector
//			  life,		//particles life duration
//			  offset		//initial position randomness, should be exploded object radius +-
//			  );

}


void testApp::mouseReleased(int x, int y, int button){

	ofVec2f target = ofVec2f(x,y);
	Silo * silo = bestSiloForTarget( target );

	if (silo != NULL){
		Missile *m = silo->launchMissile();
		if(m!=NULL){
			launch.play();
			m->startGood( target );
			playerMissiles.push_back(m);
		}
	}else{
		startLevel();
	}
}




void testApp::windowResized(int w, int h){

}


void testApp::gotMessage(ofMessage msg){

}


void testApp::dragEvent(ofDragInfo dragInfo){ 

}