FROM ubuntu:18.04

WORKDIR /app
ENV DEBIAN_FRONTEND=noninteractive

RUN sed -i -r 's|(archive\|security)\.ubuntu\.com/|ftp.jaist.ac.jp/pub/Linux/|' /etc/apt/sources.list && apt-get update && apt-get install -y python-pip && \
    apt-get upgrade -y && \
    apt-get install -y build-essential apt-utils ca-certificates \
    unzip cmake git libgtk-3-dev pkg-config \
    libswscale-dev wget autoconf automake curl \
    python-dev python-numpy python3 python3-pip python3-dev \
    libjpeg-dev libdc1394-22-dev libv4l-dev \
    libopencv-dev python-pycurl \
    libpng-dev libswscale-dev libtbb-dev libtiff-dev \ 
    libatlas-base-dev gfortran webp qt5-default libvtk6-dev zlib1g-dev \
    libxml2-dev libxslt1-dev zlib1g-dev emacs && \
    rm -rf /var/lib/apt/lists/*

RUN rm -f /usr/bin/python && ln -s /usr/bin/python3.6 /usr/bin/python
RUN rm -f /usr/local/bin/pip; ln -s /usr/local/bin/pip3.6 /usr/local/bin/pip
RUN python -m pip install --upgrade pip
RUN pip install numpy

WORKDIR /app
ENV OPENCV_VERSION="3.4.3"
RUN mkdir -p /app/opencv-$OPENCV_VERSION/build

RUN wget https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip && \
    unzip ${OPENCV_VERSION}.zip

WORKDIR /app/opencv-$OPENCV_VERSION/build
RUN cmake -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DPYTHON_EXECUTABLE=$(which python) \
    -DPYTHON_INCLUDE_DIR=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())") \
    -DPYTHON_PACKAGES_PATH=$(python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())") \
     .. 

RUN make -j install && \
    rm /app/${OPENCV_VERSION}.zip && \
    rm -r /app/opencv-${OPENCV_VERSION}

ENV GRPC_PYTHON_VERSION 1.15.0
RUN python -m pip install --upgrade pip
RUN pip install grpcio==${GRPC_PYTHON_VERSION} grpcio-tools==${GRPC_PYTHON_VERSION} grpcio-reflection==${GRPC_PYTHON_VERSION} grpcio-health-checking==${GRPC_PYTHON_VERSION} grpcio-testing==${GRPC_PYTHON_VERSION}

COPY ./requirements.txt /home
WORKDIR /home
RUN pip install -r requirements.txt

WORKDIR /app
COPY . /app

RUN mkdir -p /etc/udev/rules.d
    
RUN ./install_zense_sdk.sh

ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:/root/.local/lib:/usr/local/lib
ENV PATH $PATH:/root/.local/bin

RUN ./install_zwrapper.sh
RUN python gen_protobuf_codes.py

#For click
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

CMD ["/bin/bash"]