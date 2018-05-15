package thunt.a4over6

import android.app.Activity
import android.content.Intent
import android.net.VpnService
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_main.*
import java.io.*

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        sample_text.text = stringFromJNI()

        mainbutton.setOnClickListener{
            sample_text.text = stringclickedFromJNI(1)
            println("button clicked")
            if (! hasIpThread) {
                hasIpThread = true
                threadCPP()
            }
            sample_text.text = beginWork()
//            if (! hasStatsThread) {
//                hasStatsThread = true
//                threadStats()
//            }
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun stringclickedFromJNI(x: Int): String
    external fun startCPP(): Int
    //val JAVA_JNI_PIPE_JTOC_PATH = "/data/data/thunt.a4over6/4over6.jtoc"
    //val JAVA_JNI_PIPE_CTOJ_PATH = "/data/data/thunt.a4over6/4over6.ctoj"

    val JNI_IP_PIPE_PATH    = "/data/data/thunt.a4over6/4over6.ip"
    val JNI_STATS_PIPE_PATH = "/data/data/thunt.a4over6/4over6.stats"

    lateinit var ipPacket: IpPacket
    lateinit var ipThread: Thread
    lateinit var statsThread: Thread
    var hasIpThread: Boolean = false
    var hasStatsThread: Boolean = false

    class IpPacket(info: String) {
        val infopiece = info.split(" ")
        val ip = infopiece[0]
        val route = infopiece[1]
        val dns1 = infopiece[2]
        val dns2 = infopiece[3]
        val dns3 = infopiece[4]
    }

    class StatsPacket(info: String) {
        val infopiece = info.split(" ")
        val outlen = infopiece[0]
        val outtimes = infopiece[1]
        val inlen = infopiece[2]
        val intimes = infopiece[3]
        val isvalid = infopiece[4]
    }

    fun ReadPipe(addr: String): String? {
        val file = File(addr)
        val bufferedReader: BufferedReader = file.bufferedReader()
        val inputString = bufferedReader.use { it.readLine() }
        println("Readpipe, " + inputString)
        bufferedReader.close()
        return inputString
    }

    fun StartVPN() {
        val intent = VpnService.prepare(this)
        if (intent != null) {
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, Activity.RESULT_OK, null)
        }
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

    fun threadStats() : Thread {
        statsThread = object: Thread(){
            public override fun run() {
                updateStream()
            }
        }
        println("Start stats thread")
        statsThread.start()
        return statsThread
    }

    fun updateStream() {
        while (true) {
            var statsstring? = ReadPipe(JNI_STATS_PIPE_PATH)
            //updateStats(statsstring)
            Thread.sleep(1000)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (resultCode == Activity.RESULT_OK){
            val serviceIntent = Intent(this, MyVpnService::class.java)
            serviceIntent.putExtra("ip", ipPacket.ip)
            serviceIntent.putExtra("route", ipPacket.route)
            serviceIntent.putExtra("dns1", ipPacket.dns1)
            serviceIntent.putExtra("dns2", ipPacket.dns2)
            serviceIntent.putExtra("dns3", ipPacket.dns3)
            startService(serviceIntent)
            println("on activity result")
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    fun beginWork() : String {
        println("Begin work")
        var ipstring = ReadPipe(JNI_IP_PIPE_PATH)
        while (ipstring == null) {
            ipstring = ReadPipe(JNI_IP_PIPE_PATH)
        }
        println("Got ip")
        ipPacket = IpPacket(ipstring)
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
