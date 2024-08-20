
var xhr;
var idTimer1;
var max_cnt, cur_cnt;
var stoper;

const ip_str = ["ipa1=", "ipa2=", "ipa3=", "ipa4=",
                "ipm1=", "ipm2=", "ipm3=", "ipm4=",
                "ipg1=", "ipg2=", "ipg3=", "ipg4="];


function onload(){
    xhr = new XMLHttpRequest (); 
    load_data();
}


function onload_alarms(){
    xhr = new XMLHttpRequest ();
    load_alarms();
}

function onload_ping(){
    xhr = new XMLHttpRequest (); 
//    load_data();
}

function Timer1(){
    xhr.open("GET", "getdata", true);
    xhr.responseType = "text";
    xhr.onload = function () {
        let state = this.responseText;

        beg = state.indexOf('status=') + 7;
        end = state.length;
        if (end > beg) {
            dat = state.substring(beg, end);
        }
        else {
            dat = '-';
        }
        document.getElementById('inf6').innerHTML = dat;

        if (dat == 10) {
            stoper = 1;
            clearTimeout(idTimer1);   
            document.getElementById('ctrl_bttn').innerHTML = '<input class="submit_b" type="button" onclick="starthandler()" value="Start"</input>';
        }
        else {
            beg = state.indexOf('cnt=') + 4;
            end = state.indexOf('&lost=');
            if (end > beg) {
                cur_cnt = state.substring(beg, end);
            }
            else {
                cur_cnt = '-';
            }
            document.getElementById('inf1').innerHTML = cur_cnt;
    
            beg = end + 6;
            end = state.indexOf('&tcur=');
            if (end > beg) {
                dat = state.substring(beg, end);
            }
            else {
                dat = '-';
            }
            document.getElementById('inf2').innerHTML = dat;
    
            beg = end + 6;
            end = state.indexOf('&tmin=');
            if (end > beg) {
                dat = state.substring(beg, end);
            }
            else {
                dat = '-';
            }
            document.getElementById('inf3').innerHTML = dat;
    
            beg = end + 6;
            end = state.indexOf('&tmax=');
            if (end > beg) {
                dat = state.substring(beg, end);
            }
            else {
                dat = '-';
            }
            document.getElementById('inf4').innerHTML = dat;
    
            beg = end + 6;
            end = state.indexOf('&status=');
            if (end > beg) {
                dat = state.substring(beg, end);
            }
            else {
                dat = '-';
            }
            document.getElementById('inf5').innerHTML = dat;
    
            if (stoper == 0) {
                if (cur_cnt < (max_cnt - 1)) {
                    idTimer1 = setTimeout("Timer1()", 1000);         
                }
                else {
                    stoper = 1;
                    clearTimeout(idTimer1);   
                    document.getElementById('ctrl_bttn').innerHTML = '<input class="submit_b" type="button" onclick="starthandler()" value="Start"</input>';
                }
            }
            else {
                clearTimeout(idTimer1);   
                document.getElementById('ctrl_bttn').innerHTML = '<input class="submit_b" type="button" onclick="starthandler()" value="Start"</input>';
            }    
        }

    }
    xhr.send(null);
}


function stophandler() {
    stoper = 1;
    clearTimeout(idTimer1);   
    document.getElementById('ctrl_bttn').innerHTML = '<input class="submit_b" type="button" onclick="starthandler()" value="Start"</input>';
    xhr.open("GET", "stop", true);
    xhr.responseType = "text";
    xhr.onload = function () {
        let state = this.responseText;

        beg = state.indexOf('status=') + 7;
        end = state.length;
        if (end > beg) {
            dat = state.substring(beg, end);
        }
        else {
            dat = '-';
        }
        document.getElementById('inf6').innerHTML = dat;

        beg = state.indexOf('cnt=') + 4;
        end = state.indexOf('&lost=');
        if (end > beg) {
            cur_cnt = state.substring(beg, end);
        }
        else {
            cur_cnt = '-';
        }
        document.getElementById('inf1').innerHTML = cur_cnt;

        beg = end + 6;
        end = state.indexOf('&tcur=');
        if (end > beg) {
            dat = state.substring(beg, end);
        }
        else {
            dat = '-';
        }
        document.getElementById('inf2').innerHTML = dat;

        beg = end + 6;
        end = state.indexOf('&tmin=');
        if (end > beg) {
            dat = state.substring(beg, end);
        }
        else {
            dat = '-';
        }
        document.getElementById('inf3').innerHTML = dat;

        beg = end + 6;
        end = state.indexOf('&tmax=');
        if (end > beg) {
            dat = state.substring(beg, end);
        }
        else {
            dat = '-';
        }
        document.getElementById('inf4').innerHTML = dat;

        beg = end + 6;
        end = state.indexOf('&status=');
        if (end > beg) {
            dat = state.substring(beg, end);
        }
        else {
            dat = '-';
        }
        document.getElementById('inf5').innerHTML = dat;
    }
    xhr.send(null);
}


function starthandler() {
    max_cnt = document.getElementsByName('amnt')[0].value;
    xhr.open("GET", "start?" + 
        "&hipa1=" + document.getElementById('hipa1').value +
        "&hipa2=" + document.getElementById('hipa2').value +
        "&hipa3=" + document.getElementById('hipa3').value + 
        "&hipa4=" + document.getElementById('hipa4').value +
        "&n=" + max_cnt, true);
    xhr.responseType = "text";
    xhr.onload = function () {
        let state = this.responseText;
        if (state.includes('state=OK')) {
            document.getElementById('ctrl_bttn').innerHTML = '<input class="submit_b" type="button" onclick="stophandler()" value="Stop"</input>';
            Timer1();
            stoper = 0;
        }
        else {
            window.alert('Server is busy');
        }
    }
    xhr.send(null);        
}


function led_on(){
    xhr.open("GET", "switch_led?c=1", true);
//    xhr.responseType = "text";
    xhr.send(null);
}


function led_off(){
    xhr.open("GET", "switch_led?c=0", true);
//    xhr.responseType = "text";
    xhr.send(null);
}


function submit_netsettings(){
    if (document.getElementsByName('ip_assign')[0].checked == true) {        
        xhr.open("GET", "setconfig?ip=static" +
            "&ipa1=" + document.getElementById('ipa1').value +
            "&ipa2=" + document.getElementById('ipa2').value +
            "&ipa3=" + document.getElementById('ipa3').value + 
            "&ipa4=" + document.getElementById('ipa4').value +
            "&ipm1=" + document.getElementById('ipm1').value + 
            "&ipm2=" + document.getElementById('ipm2').value +
            "&ipm3=" + document.getElementById('ipm3').value + 
            "&ipm4=" + document.getElementById('ipm4').value +
            "&ipg1=" + document.getElementById('ipg1').value + 
            "&ipg2=" + document.getElementById('ipg2').value +
            "&ipg3=" + document.getElementById('ipg3').value + 
            "&ipg4=" + document.getElementById('ipg4').value +
            "&ssid=" + document.getElementById('ssid').value + 
            "&psw=" + document.getElementById('psw').value, true);
    } else {
        xhr.open("GET", "setconfig?ip=dhcp" + 
            "&ssid=" + document.getElementById('ssid').value + 
            "&psw=" + document.getElementById('psw').value, true);
    }
    xhr.send(null);
}


function submit_alarm(){
    xhr.open("GET", "setalarm?unreplied=" + document.getElementsByName('unrepl')[0].value +
        "&email=" + document.getElementsByName('email')[0].value +
        "&telega=" + document.getElementsByName('telega')[0].value + 
        "&red=" + document.getElementsByName('red')[0].checked +
        "&blue=" + document.getElementsByName('blue')[0].checked + 
        "&green=" + document.getElementsByName('green')[0].checked, true);
    xhr.send(null); 
    
    // document.getElementById('demo3').innerHTML = "setalarm?unreplied=" + document.getElementsByName('unrepl')[0].value +
    //     "&email=" + document.getElementsByName('email')[0].value +
    //     "&telega=" + document.getElementsByName('telega')[0].value + 
    //     "&red=" + document.getElementsByName('red')[0].checked +
    //     "&blue=" + document.getElementsByName('blue')[0].checked + 
    //     "&green=" + document.getElementsByName('green')[0].checked;
}


function set_dhcp(){
    document.getElementById('stat_ip').style.display = 'none';
}


function set_static(){
    document.getElementById('stat_ip').style.display = 'block';
}


function load_data(){
    xhr.onload = function() {
//    xhr.onreadystatechange = function() {
        let state = this.responseText;
        if (state == "") {
            return;
        }         
        if (state.includes('ip=static')) {
            document.getElementsByName('ip_assign')[0].checked=true;
            set_static();
            for (let i = 0; i < 12; i++) {
                beg = state.indexOf(ip_str[i]) + 5;
                if (i == 11) {
                    end = state.length;
                }
                else {
                    end = state.indexOf(ip_str[i+1]) - 1;
                }
                dat = state.substring(beg, end);
                document.getElementById(ip_str[i].substring(0, 4)).value = dat;
            }
        }        
        else if (state.includes('ip=dhcp')) {
            document.getElementsByName('ip_assign')[1].checked=true;
            set_dhcp();
        }
        document.getElementById('demo1').innerHTML = state;
        document.getElementById('demo2').innerHTML = dat;
    }
    xhr.open("GET", "getconfig", true);
    xhr.send(null);
}


function load_alarms(){
    xhr.onload = function() {
        let state = this.responseText;
        if (state == "") {
            return;
        }         

        beg = state.indexOf('unreplied=') + 10;
        end = state.indexOf('&email=');
        if (end > beg) {
            dat = state.substring(beg, end);
            if (dat == 0) {
                dat = 5
            }
        }
        else {
            dat = 5;
        }
        document.getElementsByName('unrepl')[0].value = dat;

        beg = end + 7;
        end = state.indexOf('&telega=');
        if (end > beg) {
            dat = state.substring(beg, end);
            document.getElementsByName('email')[0].value = dat;
        }

        beg = end + 8;
        end = state.indexOf('&red=');
        if (end > beg) {
            dat = state.substring(beg, end);
            document.getElementsByName('telega')[0].value = dat;
        }
        
        if (state.includes('red=true')) {
            document.getElementsByName('red')[0].checked=true;
        }
        if (state.includes('blue=true')) {
            document.getElementsByName('blue')[0].checked=true;
        }
        if (state.includes('green=true')) {
            document.getElementsByName('green')[0].checked=true;
        }

        document.getElementById('demo3').innerHTML = state;
        document.getElementById('demo4').innerHTML = dat;
    }
    xhr.open("GET", "getalarms", true);
    xhr.send(null);    
}