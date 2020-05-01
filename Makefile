.PHONY: build
build:
	sudo docker build -t grpc_test .

.PHONY: rebuild
rebuild:
	sudo docker build --no-cache -t grpc_test .

.PHONY: run
run:
	sudo docker run -it --rm\
    --network="host" \
	--mount type=bind,src=$(PWD),dst=/home \
	 grpc_test ipython