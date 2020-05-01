FROM python:3.6

ENV GRPC_PYTHON_VERSION 1.15.0
RUN python -m pip install --upgrade pip
RUN pip install grpcio==${GRPC_PYTHON_VERSION} grpcio-tools==${GRPC_PYTHON_VERSION} grpcio-reflection==${GRPC_PYTHON_VERSION} grpcio-health-checking==${GRPC_PYTHON_VERSION} grpcio-testing==${GRPC_PYTHON_VERSION}

COPY ./requirements.txt /home
WORKDIR /home
RUN pip install -r requirements.txt