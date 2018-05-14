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
            if (! hasThread) {
                hasThread = true
                threadCPP()
            }
            sample_text.text = beginWork()
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun stringclickedFromJNI(x: Int): String
    external fun startCPP(): Int
    val JAVA_JNI_PIPE_JTOC_PATH = "/data/data/thunt.a4over6/4over6.jtoc"
    val JAVA_JNI_PIPE_CTOJ_PATH = "/data/data/thunt.a4over6/4over6.ctoj"

    val JNI_IP_PIPE_PATH    = "/data/data/thunt.a4over6/4over6.ip"
    val JNI_STATS_PIPE_PATH = "/data/data/thunt.a4over6/4over6.stats"

    lateinit var ipPacket: IpPacket
    var hasThread: Boolean = false

    class IpPacket(info: String) {
        val infopiece = info.split(" ")
        val ip = infopiece[0]
        val route = infopiece[1]
        val dns1 = infopiece[2]
        val dns2 = infopiece[3]
        val dns3 = infopiece[4]
        val socket = infopiece[5]
    }

    fun ReadPipe(): String? {
        val file = File(JNI_IP_PIPE_PATH)
        val bufferedReader: BufferedReader = file.bufferedReader()
        println("Readpipebegin")
        val inputString = bufferedReader.use { it.readLine() }
        println("Readpipe, " + inputString)
        bufferedReader.close()
        return inputString
    }

    fun StartVPN() {
        val intent = VpnService.prepare(this)
        if (intent != null) {
            println("intent not null")
            startActivityForResult(intent, 0);
        } else {
            println("intent null")
            onActivityResult(0, Activity.RESULT_OK, null)
        }
    }

    fun threadCPP() : Thread {
        val thread = object: Thread(){
            public override fun run() {
                startCPP()
            }
        }
        println("Start thread")
        thread.start()
        println("Start after thread")
        return thread
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (resultCode == Activity.RESULT_OK){
            val serviceIntent = Intent(this, MyVpnService::class.java)
            serviceIntent.putExtra("ip", ipPacket.ip)
            serviceIntent.putExtra("route", ipPacket.route)
            serviceIntent.putExtra("dns1", ipPacket.dns1)
            serviceIntent.putExtra("dns2", ipPacket.dns2)
            serviceIntent.putExtra("dns3", ipPacket.dns3)
            serviceIntent.putExtra("socket", ipPacket.socket)
            println("on activity result")
            startService(serviceIntent)
            println("after activity result")
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    fun beginWork() : String {
        println("Begin work")
        var ipstring = ReadPipe()
        while (ipstring == null) {
            ipstring = ReadPipe()
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
