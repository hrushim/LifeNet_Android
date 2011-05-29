package com.app.lifeNet;

import java.io.Serializable;
import java.util.Arrays;

public class ChatMessage implements Serializable {

    static final long serialVersionUID = 3000;
    public int type;
    public String srcName;
    public String destName;
    public long seq;
    String payload;

    public ChatMessage(String src_name, String dest_name, long seq_num, long payload_length, String pay_load, int m_type) {

        srcName = src_name;
        destName = dest_name;
        seq = seq_num;
        payload = pay_load;
        type = m_type;
    }

    /*
    public void printData() {
            System.out.println("==== NetMessage Object ====");
            System.out.println("SrcIP: " + String.valueOf(srcIP));
            System.out.println("DestIP " + String.valueOf(destIP));
            System.out.println("SeqNum: " + seq);
            System.out.println("Type: " + type);
            System.out.println("Payload: " + new String(payload));
            System.out.println("==== End NetMessage Object ====");
            System.out.flush();
    }*/

}
