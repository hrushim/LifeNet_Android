package com.app.lifeNet;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

public class UdpTx {

	public static void send(String destIp, int port, ChatMessage msg) {

		InetAddress ipAdd = null;
		DatagramPacket packet = null;
		ByteArrayOutputStream byteStream = null;
		ObjectOutputStream oStream = null;
		byte[] buf;
		DatagramSocket socketSend = null;

		if (msg != null) {

			try {
				socketSend = new DatagramSocket();
			} catch (SocketException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			try {
				ipAdd = InetAddress.getByName(destIp);
			} catch (UnknownHostException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			byteStream = new ByteArrayOutputStream();
			if (byteStream != null) {

				try {
					oStream = new ObjectOutputStream(byteStream);
					oStream.writeObject(msg);
					oStream.flush();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				
				buf = byteStream.toByteArray();
				//buf = new BASE64Encoder().encode(buf).getBytes();
				packet = new DatagramPacket(buf, buf.length, ipAdd, port);
				
				if(packet!=null) {
					
					try {
						socketSend.send(packet);
						byteStream.flush();
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			}
		}
	}
}
