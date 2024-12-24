from debian:latest

RUN apt update && apt install -y python3 python3-pip
RUN pip3 install asyncio --break-system-packages
ADD tarpit.py /root/

CMD /root/tarpit.py
