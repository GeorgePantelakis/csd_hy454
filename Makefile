all: superMario

superMario: Animation.o Animator.o app.o Bitmap.o BoundingArea.o Color.cpp createNPCs.o Destruction.o GravityHandler.o GridLayer.o opperations.o Sprite.o SystemClock.o TileLayer.o Utilities.o main.cpp
	g++ $^ -lallegro-5.0.10-mt -lallegro_image-5.0.10-mt -o $@

%.o: ./Src/%.cpp ./Include/%.h
	g++ -c $< -lallegro-5.0.10-mt -lallegro_image-5.0.10-mt -o $@

clean:
	rm -rf *.o superMario