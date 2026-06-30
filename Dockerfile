# Based off this: https://gist.github.com/bwrrp/dc2fe8926dfe8860da21cb87ba91aeaa
FROM espressif/idf:v6.0
RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc