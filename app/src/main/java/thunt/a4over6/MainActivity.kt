package thunt.a4over6

import android.app.Activity
import android.content.Intent
import android.net.VpnService
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_main.*
import java.io.*
import java.net.Inet6Address
import java.net.NetworkInterface
import java.util.*

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        sample_text.text = stringFromJNI()

        mainbutton.setOnClickListener{
            sample_text.text = stringclickedFromJNI(1)
            println("button clicked")
            if (! connected) {
                connected = true
                mainbutton.text = "STOP"
                if (!hasMainThread) {
                    hasMainThread = true
                    threadMain()
                }
            }
            else {
                connected = false
                mainbutton.text = "CONNECT"
                hasIpThread = false
                WritePipe_stop()
                textView_ipv4_addr.text = "IPV4 address: "
                textView_ipv6_addr.text = "IPV6 address: "
                textView_upload_speed.text = "upload spped: 0 B/s"
                textView_download_speed.text = "download speed: 0 B/s"
                textView_time_duration.text = "time duration: 00:00:00"
            }
        }
    }

    external fun stringFromJNI(): String
    external fun stringclickedFromJNI(x: Int): String
    external fun startCPP(): Int

    val JNI_IP_PIPE_PATH    = "/data/data/thunt.a4over6/4over6.ip"
    val JNI_STATS_PIPE_PATH = "/data/data/thunt.a4over6/4over6.stats"
    val JNI_DES_PIPE_PATH   = "/data/data/thunt.a4over6/4over6.des"

    lateinit var ipPacket: IpPacket
    lateinit var ipThread: Thread
    lateinit var statsThread: Thread
    lateinit var mainThread: Thread
    lateinit var beginTime: Date
    lateinit var serviceIntent: Intent
    var hasIpThread: Boolean = false
    var hasStatsThread: Boolean = false
    var hasMainThread: Boolean = false
    var connected: Boolean = false
    var upload_flow: Long = 0
    var download_flow: Long = 0
    var upload_packets: Long = 0
    var download_packets: Long = 0

    class IpPacket(info: String) {
        val infopiece = info.split(" ")
        val ip = infopiece[0].trim()
        val route = infopiece[1].trim()
        val dns1 = infopiece[2].trim()
        val dns2 = infopiece[3].trim()
        val dns3 = infopiece[4].trim()
        val socket = infopiece[5].trim()
    }

    class StatsPacket(info: String) {
        val infopiece = info.split(" ")
        val outlen = infopiece[0].trim()
        val outtimes = infopiece[1].trim()
        val inlen = infopiece[2].trim()
        val intimes = infopiece[3].trim()
        val isvalid = infopiece[4].trim()
    }

    fun ReadPipe(addr: String): String? {
        val file = File(addr)
        val bufferedReader: BufferedReader = file.bufferedReader()
        val inputString = bufferedReader.use { it.readLine() }
        println("Readpipe, " + inputString)
        bufferedReader.close()
        return inputString
    }

    fun WritePipe_stop() {
        val file = File(JNI_DES_PIPE_PATH)
        val bufferWriter: BufferedWriter = file.bufferedWriter()
        bufferWriter.use { out->out.write("q1j") }
        bufferWriter.close()
    }

    fun StartVPN() {
        val intent = VpnService.prepare(this)
        if (intent != null) {
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, Activity.RESULT_OK, null)
        }
    }

    fun threadMain() : Thread {
        mainThread = object: Thread(){
            public override fun run() {
                if (! hasIpThread) {
                    hasIpThread = true
                    threadCPP()
                }
                runOnUiThread { sample_text.text = beginWork() }
                if (! hasStatsThread) {
                    hasStatsThread = true
                    threadStats()
                }
                hasMainThread = false
            }
        }
        println("Start thread")
        mainThread.start()
        return mainThread
    }

    fun threadCPP() : Thread {
        ipThread = object: Thread(){
            public override fun run() {
                startCPP()
            }
        }
        println("Start thread")
        ipThread.start()
        return ipThread
    }

    fun toString2(x: Long): String {
        if (x < 10)
            return "0" + x.toString()
        return x.toString()
    }

    fun toKB(x: Long): String {
        if (x < 1024)
            return x.toString() + " B"
        return (x / 1024).toString() + " KB"
    }

    fun threadStats() : Thread {
        statsThread = object: Thread(){
            public override fun run() {
                try {
                    beginTime = Date()
                    while (true) {
                        var statsstring = ReadPipe(JNI_STATS_PIPE_PATH)
                        runOnUiThread { sample_text.text = statsstring }
                        if (statsstring == null)
                            statsstring = "0 0 0 0 0"
                        var statsPackets = StatsPacket(statsstring)
                        if (statsPackets.isvalid == "-1") {
                            statsPackets = StatsPacket("0 0 0 0 0")
                            break
                        }
                        upload_flow += statsPackets.inlen.toLong()
                        upload_packets += statsPackets.intimes.toLong()
                        download_flow += statsPackets.outlen.toLong()
                        download_packets += statsPackets.outtimes.toLong()
                        var timeDuration = (Date().time - beginTime.time) / 1000
                        var hour = timeDuration / 3600
                        var minute = (timeDuration % 3600) / 60
                        var second = timeDuration % 60
                        runOnUiThread {
                            textView_time_duration.text = "time duration: " + toString2(hour) + ":" + toString2(minute) + ":" + toString2(second)
                            textView_upload_speed.text = "upload spped: " + toKB(statsPackets.inlen.toLong()) + "/s"
                            textView_download_speed.text = "download speed: " + toKB(statsPackets.outlen.toLong()) + "/s"
                            textView_upload_flow.text = "upload flow: " + toKB(upload_flow)
                            textView_download_flow.text = "download flow: " + toKB(download_flow)
                            textView_upload_packets.text = "upload packets: " + upload_packets.toString()
                            textView_download_packets.text = "download packets: " + download_packets.toString()
                        }
                        Thread.sleep(750)
                    }
                    statsThread.interrupt()
                    stopService(serviceIntent)
                    hasStatsThread = false
                } catch (e: InterruptedException) {
                    println("interrupted")
                }
            }
        }
        println("Start stats thread")
        statsThread.start()
        return statsThread
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (resultCode == Activity.RESULT_OK){
            serviceIntent = Intent(this, MyVpnService::class.java)
            serviceIntent.putExtra("ip", ipPacket.ip)
            serviceIntent.putExtra("route", ipPacket.route)
            serviceIntent.putExtra("dns1", ipPacket.dns1)
            serviceIntent.putExtra("dns2", ipPacket.dns2)
            serviceIntent.putExtra("dns3", ipPacket.dns3)
            serviceIntent.putExtra("socket", ipPacket.socket)
            println("on activity result")
            startService(serviceIntent)
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    fun getLocalIpAddress(): String {
        var en = NetworkInterface.getNetworkInterfaces()
        while (en.hasMoreElements()) {
            var intf = en.nextElement()
            var enumIpAddr = intf.inetAddresses
            while (enumIpAddr.hasMoreElements()) {
                var inetAddress = enumIpAddr.nextElement()
                if (!inetAddress.isLoopbackAddress && inetAddress is Inet6Address)
                    return inetAddress.hostAddress
            }
        }
        return ""
    }

    fun beginWork() : String {
        println("Begin work")
        var ipstring = ReadPipe(JNI_IP_PIPE_PATH)
        while (ipstring == null) {
            ipstring = ReadPipe(JNI_IP_PIPE_PATH)
        }
        println("Got ip")
        ipPacket = IpPacket(ipstring)
        val ipv6 = getLocalIpAddress()
        runOnUiThread {
            textView_ipv4_addr.text = "IPV4 address: " + ipPacket.ip
            textView_ipv6_addr.text = "IPV6 address: " + ipv6
        }
        StartVPN()
        return ipstring
    }

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
