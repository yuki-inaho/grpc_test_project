.PHONY: build
build:
	sudo docker build -t grpc_test .

.PHONY: rebuild
rebuild:
	sudo docker build --no-cache -t grpc_test .

.PHONY: run
run:
	xhost + local:root
	sudo docker run -it \
    --network="host" \
	--env=DISPLAY=$(DISPLAY) \
	--env=QT_X11_NO_MITSHM=1 \
	--privileged \
	--mount type=bind,src=$(PWD),dst=/app \
	--mount type=bind,src=/dev,dst=/dev,readonly \
	--volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
	 grpc_test /bin/bash