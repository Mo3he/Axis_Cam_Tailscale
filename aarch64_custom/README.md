To build, from main directory

docker build --tag aarch64 . 

docker cp $(docker create aarch64):/opt/app ./build 