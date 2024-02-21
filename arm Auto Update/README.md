To build from main directory

docker build --tag arm . 

docker cp $(docker create arm):/opt/app ./build 