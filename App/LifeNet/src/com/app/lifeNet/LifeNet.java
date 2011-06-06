package com.app.lifeNet;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.Timer;
import java.util.Vector;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings.SettingNotFoundException;
import android.provider.Settings.System;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class LifeNet extends Activity implements Runnable {

	public static int MSG_TYPE_DATA = 1;
	public static int MSG_TYPE_ACK = 2;
	public static int MSG_SRC_DATA_PORT = 30000;
	public static int MSG_SRC_ACK_PORT = 30001;
	public static int MSG_RCV_TIMEOUT = 100;
	public static int THREAD_CNTRL_INTERVAL = 50;
	public static long SEQ_CNT = 0; // Used for sequencing data packets
	public static int MAX_NUM_OF_RX_MSGS = 255;
    public static int MSG_START_REP_CNT = 50;
    public static int MSG_REP_CNT = 5;
	public static String selectedUserName; // Not sure
	static String HOSTS_FILE_NAME = "/sdcard/hosts.txt";
	static String GNST_FILE_NAME = "/sdcard/gnst.txt";
	static String MANIFOLD_FILE_NAME = "/proc/txstats";
	boolean RUN_FLAG = false;
	Vector<String> strVector;
	Timer refreshTimer;
	WifiManager globalWifiManager;
	WifiLock globalWifiLock;
	ListView lv1;
	TextView msgsView;
	TextView inboxView;
	ArrayAdapter<String> adp;
	File manifoldFile;
	Button but;
	Thread updateThread;
	MessageThread messTh;
	LifeNet thisPtr;
	boolean wifiStaticStatus;
	boolean tempLoadFlag;
	String wifiStaticIp;
	String wifiDns1;
	String wifiDns2;
	String wifiGwIp;
	String wifiNetmask;
	public static String setUserName;
	public static String setWifiChannel;
	public static String setNetworkName;
	public static String setIp;
	int settingsFileBufferSize = 1024;

	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		readSettings();

		thisPtr = this;

		manifoldFile = new File(MANIFOLD_FILE_NAME);

		LifeNetApi.init(HOSTS_FILE_NAME, GNST_FILE_NAME);

		globalWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
		globalWifiLock = globalWifiManager.createWifiLock("G_LOCK");

		strVector = new Vector<String>();
		// strVector.add("No contacts");

		but = (Button) findViewById(R.id.button1);
		but.setOnClickListener(settingsClickListener);

		msgsView = (TextView) findViewById(R.id.textView2);
		msgsView.setOnClickListener(msgsClickListener);
		inboxView = (TextView) findViewById(R.id.textView3);
		inboxView.setOnClickListener(inboxClickListener);
		
		CheckBox cBoxWiFi = (CheckBox) findViewById(R.id.checkBox1);
		cBoxWiFi.setChecked(checkWifi());

		CheckBox cBoxLifeNet = (CheckBox) findViewById(R.id.checkBox2);
		cBoxLifeNet.setChecked(checkLifeNet());

		cBoxWiFi.setOnClickListener(wifiClickListener);
		cBoxLifeNet.setOnClickListener(lifeNetClickListener);

		lv1 = (ListView) findViewById(R.id.ListView1);
		lv1.setOnItemClickListener(listClickListener);
		adp = new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1);
		lv1.setAdapter(adp);
		adp.add("Nobody");
		lv1.setOnItemClickListener(listClickListener);

		updateAliveContacts();

		updateThread = new Thread(this);
		updateThread.start();
		setRunning(true);

		messTh = new MessageThread(this, THREAD_CNTRL_INTERVAL);
		messTh.setRunning(true);
		messTh.start();

	}

	private void readSettings() {
		try {

			byte[] byteStr = new byte[settingsFileBufferSize];

			FileInputStream fIps = new FileInputStream(
					Settings.settingsFileLocation);

			try {
				fIps.read(byteStr);
				String fileTextString = new String(byteStr);

				String strBuffer = fileTextString.toString();
				String[] strArr = strBuffer.split(";");
				setUserName = strArr[0];
				setNetworkName = strArr[1];
				setIp = strArr[3];
				setWifiChannel = strArr[2];

			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			try {
				fIps.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			// e.printStackTrace();
		}
	}

	public Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg) {

			ChatMessage chatMsg = null;
			
			CheckBox cBoxWiFi = (CheckBox) findViewById(R.id.checkBox1);
			cBoxWiFi.setChecked(checkWifi());

			CheckBox cBoxLifeNet = (CheckBox) findViewById(R.id.checkBox2);
			cBoxLifeNet.setChecked(checkLifeNet());

			updateAliveContacts();

			TextView tv = (TextView) findViewById(R.id.textView2);

			if (MessageQueue.newMsgReadFlag == true) {
				for (int i = 0; i < MessageQueue.unreadCount; i++) {
			
					chatMsg = (ChatMessage) MessageQueue.unreadMessageVector.elementAt(i);
					ChatMessage chatMsgNew = new ChatMessage(chatMsg.srcName, chatMsg.seq, chatMsg.payload.length(),  chatMsg.payload, chatMsg.type);
					chatMsgNew.rxTime = chatMsg.rxTime;
					MessageQueue.readMessageVector.add(chatMsgNew);
					MessageQueue.readCount++;
					
				}
				MessageQueue.unreadCount = 0;
				MessageQueue.newMsgReadFlag = false;
				MessageQueue.unreadMessageVector.clear();
			}

			if (MessageQueue.unreadCount > 0) {

				if (MessageQueue.unreadCount == 1) {
					tv.setText("1 Unread Message");
				} else {
					tv.setText(MessageQueue.unreadCount + " Unread Messages");
				}
			} else {
				tv.setText("No new messages");
			}
			
			inboxView.setText("Inbox(" + MessageQueue.readCount + ")");
		}

	};

	private View.OnClickListener inboxClickListener = new View.OnClickListener() {

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			Intent newIntent = new Intent(thisPtr, InboxWindow.class);
			thisPtr.startActivity(newIntent);
		}
	};

	
	private View.OnClickListener msgsClickListener = new View.OnClickListener() {

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			Intent newIntent = new Intent(thisPtr, InboxView.class);
			thisPtr.startActivity(newIntent);
		}
	};

	private OnItemClickListener listClickListener = new OnItemClickListener() {

		@Override
		public void onItemClick(AdapterView<?> arg0, View arg1, int arg2,
				long arg3) {
			// TODO Auto-generated method stub
			// Toast.makeText(getApplicationContext(), adp.getItem(arg2),
			// Toast.LENGTH_SHORT).show();
			selectedUserName = adp.getItem(arg2);
			Intent newIntent = new Intent(thisPtr, MessageWindow.class);
			thisPtr.startActivity(newIntent);
		}
	};

	private View.OnClickListener settingsClickListener = new OnClickListener() {

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			Intent newIntent = new Intent(thisPtr, Settings.class);
			thisPtr.startActivity(newIntent);
		}
	};

	private View.OnClickListener lifeNetClickListener = new OnClickListener() {
		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub

			CheckBox cBox = (CheckBox) v;

			readSettings();

			if (cBox.isChecked()) {

				if (checkWifi()) {

					Process process;
					try {
						process = Runtime.getRuntime().exec(
								"su -c \"/data/data/start_manifold.sh\"");
					} catch (IOException e) { // TODO Auto-generated catch block
												// e.printStackTrace(); }

					}
					updateAliveContacts();
				} else {
					Toast.makeText(getApplicationContext(),
							"Enable wifi before starting LifeNet",
							Toast.LENGTH_SHORT).show();
				}

			} else {
				Process process;
				try {
					process = Runtime.getRuntime().exec(
							"su -c \"/data/data/stop_manifold.sh\"");
				} catch (IOException e) { // TODO Auto-generated catch block
											// e.printStackTrace(); }

				}
				updateAliveContacts();
			}
		}

	};

	private View.OnClickListener wifiClickListener = new OnClickListener() {
		@Override
		public void onClick(View v) {

			CheckBox cBox = (CheckBox) v;

			readSettings();

			if (cBox.isChecked()) {

				try {
					if (System.getInt(getContentResolver(),
							System.WIFI_USE_STATIC_IP) == 1) {
						wifiStaticStatus = true;
						wifiStaticIp = System.getString(getContentResolver(),
								System.WIFI_STATIC_IP);
						wifiDns1 = System.getString(getContentResolver(),
								System.WIFI_STATIC_DNS1);
						wifiDns2 = System.getString(getContentResolver(),
								System.WIFI_STATIC_DNS2);
						wifiNetmask = System.getString(getContentResolver(),
								System.WIFI_STATIC_NETMASK);
						wifiGwIp = System.getString(getContentResolver(),
								System.WIFI_STATIC_GATEWAY);
					} else
						wifiStaticStatus = false;

					int useStaticIp = 1;
					System.putInt(getContentResolver(),
							System.WIFI_USE_STATIC_IP, useStaticIp);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_IP, setIp);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_NETMASK, "255.255.255.0");
					System.putString(getContentResolver(),
							System.WIFI_STATIC_GATEWAY, "192.168.0.1");
					System.putString(getContentResolver(),
							System.WIFI_STATIC_DNS1, "0.0.0.0");
					System.putString(getContentResolver(),
							System.WIFI_STATIC_DNS2, "0.0.0.0");

				} catch (SettingNotFoundException e) {
				}

				Process process = null;
				try {
					process = Runtime
							.getRuntime()
							.exec("su -c \"/data/data/com.app.lifeNet/load_tiwlan.sh\"");
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				try {
					process.waitFor();
				} catch (InterruptedException e) {
					e.printStackTrace();
				}

				WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
				globalWifiLock.acquire();
				wifi.setWifiEnabled(true);
				globalWifiLock.release();

			} else {

				Process process = null;
				try {
					process = Runtime
							.getRuntime()
							.exec("su -c \"/data/data/com.app.lifeNet/unload_tiwlan.sh\"");
				} catch (IOException e) {
					e.printStackTrace();
				}
				try {
					process.waitFor();
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}

				if (wifiStaticStatus == false) {

					int useStaticIp = 0;
					System.putInt(getContentResolver(),
							System.WIFI_USE_STATIC_IP, useStaticIp);

				} else {

					int useStaticIp = 1;
					System.putInt(getContentResolver(),
							System.WIFI_USE_STATIC_IP, useStaticIp);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_IP, wifiStaticIp);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_NETMASK, wifiNetmask);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_GATEWAY, wifiGwIp);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_DNS1, wifiDns1);
					System.putString(getContentResolver(),
							System.WIFI_STATIC_DNS2, wifiDns2);

				}

				WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
				globalWifiLock.acquire();
				wifi.setWifiEnabled(false);
				globalWifiLock.release();
			}
			updateAliveContacts();
		}
	};

	public void updateAliveContacts() {

		adp.clear();
		adp.notifyDataSetChanged();

		StringBuilder sb = new StringBuilder();
		try {

			BufferedReader in = new BufferedReader(new FileReader(
					HOSTS_FILE_NAME));
			String str;

			int i = 0;
			while ((str = in.readLine()) != null) {
				String[] strArr = str.split(" ");
				sb.append(strArr[3]);
				// strVector.add(strArr[3]);
				if (LifeNetApi.getMyName().trim().equals(strArr[3])) {
					adp.add(strArr[3]);
				} else {
					adp.add(strArr[3]
							+ "    [(ED = "
							+ LifeNetApi.getED(LifeNetApi.getMyName(),
									strArr[3])
							+ "), ("
							+ LifeNetApi.getNumTxNumRx(LifeNetApi.getMyName(),
									strArr[3]) + ")]");
				}
				sb.append(" ");
			}
			in.close();

		} catch (Exception e) {
			sb.append("<Disconnected>");
		}
		if (adp.isEmpty()) {
			adp.add("<Nobody online>");
		}

		adp.notifyDataSetChanged();

	}

	private boolean checkLifeNet() {
		return manifoldFile.exists();
	}

	private boolean checkWifi() {
		if (globalWifiManager.getWifiState() == 0x01
				|| globalWifiManager.getWifiState() == 0x00)
			return false;
		else
			return true;
	}

	@Override
	public void run() {
		// TODO Auto-generated method stub
		while (true) {
			try {
				Thread.sleep(5000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			handler.sendEmptyMessage(0);
		}
	}
	
    public void setRunning(boolean flag) {
    	RUN_FLAG = flag;
    }

    public boolean isRunning() {
    	return RUN_FLAG;
    }
}
