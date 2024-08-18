
        Highcharts.setOptions({
            lang: {
                months: ['Enero', 'Febrero', 'Marzo', 'Abril', 'Mayo', 'Junio', 'Julio', 'Agosto', 'Septiembre', 'Octubre', 'Noviembre', 'Diciembre'],
                weekdays: ['Domingo', 'Lunes', 'Martes', 'Miércoles', 'Jueves', 'Viernes', 'Sábado'],
                shortMonths: ['Ene', 'Feb', 'Mar', 'Abr', 'May', 'Jun', 'Jul', 'Ago', 'Sep', 'Oct', 'Nov', 'Dic'],
                rangeSelectorFrom: 'Desde',
                rangeSelectorTo: 'Hasta',
                rangeSelectorZoom: 'Período',
                loading: 'Cargando...',
                viewFullscreen: 'Ver en pantalla completa',
                exitFullscreen: 'Salir de pantalla completa',
                printChart: 'Imprimir gráfico',
                downloadPNG: 'Descargar PNG',
                downloadJPEG: 'Descargar JPEG',
                downloadPDF: 'Descargar PDF',
                downloadSVG: 'Descargar SVG',
                contextButtonTitle: 'Menú contextual del gráfico',
                downloadCSV: 'Descargar CSV',
                downloadXLS: 'Descargar XLS',
                viewData: 'Ver tabla de datos',
                hideData: 'Ocultar tabla de datos',
                openInCloud: 'Abrir en Highcharts Cloud',
                invalidDate: 'Fecha inválida'
            }
        });
        
        
        
        Highcharts.theme = {
            colors: ['#2b908f', '#90ee7e', '#f45b5b', '#7798BF', '#aaeeee', '#ff0066', '#eeaaee',
                '#55BF3B', '#DF5353', '#7798BF', '#aaeeee'],
            chart: {
                backgroundColor: {
                    linearGradient: { x1: 0, x2: 1, y1: 0, y2: 1 },
                    stops: [
                        [0, '#2a2a2b'],
                        [1, '#3e3e40']
                    ]
                },
                style: {
                    fontFamily: '\'Unica One\', sans-serif'
                },
                plotBorderColor: '#606063'
            },
            title: {
                style: {
                    color: '#E0E0E3',
                    textTransform: 'uppercase',
                    fontSize: '20px'
                }
            },
            subtitle: {
                style: {
                    color: '#E0E0E3',
                    textTransform: 'uppercase'
                }
            },
            xAxis: {
                gridLineColor: '#707073',
                labels: {
                    style: {
                        color: '#E0E0E3'
                    }
                },
                lineColor: '#707073',
                minorGridLineColor: '#505053',
                tickColor: '#707073',
                title: {
                    style: {
                        color: '#A0A0A3'
                    }
                }
            },
            yAxis: {
                gridLineColor: '#707073',
                labels: {
                    style: {
                        color: '#E0E0E3'
                    }
                },
                lineColor: '#707073',
                minorGridLineColor: '#505053',
                tickColor: '#707073',
                tickWidth: 1,
                title: {
                    style: {
                        color: '#A0A0A3'
                    }
                }
            },
            tooltip: {
                backgroundColor: 'rgba(0, 0, 0, 0.85)',
                style: {
                    color: '#F0F0F0'
                }
            },
            plotOptions: {
                series: {
                    dataLabels: {
                        color: '#B0B0B3'
                    },
                    marker: {
                        lineColor: '#333'
                    }
                },
                boxplot: {
                    fillColor: '#505053'
                },
                candlestick: {
                    lineColor: 'white'
                },
                errorbar: {
                    color: 'white'
                }
            },
            legend: {
                backgroundColor: 'rgba(0, 0, 0, 0.5)',
                itemStyle: {
                    color: '#E0E0E3'
                },
                itemHoverStyle: {
                    color: '#FFF'
                },
                itemHiddenStyle: {
                    color: '#606063'
                },
                title: {
                    style: {
                        color: '#C0C0C0'
                    }
                }
            },
            credits: {
                style: {
                    color: '#666'
                }
            },
            labels: {
                style: {
                    color: '#707073'
                }
            },
            drilldown: {
                activeAxisLabelStyle: {
                    color: '#F0F0F3'
                },
                activeDataLabelStyle: {
                    color: '#F0F0F3'
                }
            },
            navigation: {
                buttonOptions: {
                    symbolStroke: '#DDDDDD',
                    theme: {
                        fill: '#505053'
                    }
                }
            },
            // scroll charts
            rangeSelector: {
                buttonTheme: {
                    fill: '#505053',
                    stroke: '#000000',
                    style: {
                        color: '#CCC'
                    },
                    states: {
                        hover: {
                            fill: '#707073',
                            stroke: '#000000',
                            style: {
                                color: 'white'
                            }
                        },
                        select: {
                            fill: '#000003',
                            stroke: '#000000',
                            style: {
                                color: 'white'
                            }
                        }
                    }
                },
                inputBoxBorderColor: '#505053',
                inputStyle: {
                    backgroundColor: '#333',
                    color: 'silver'
                },
                labelStyle: {
                    color: 'silver'
                }
            },
            navigator: {
                handles: {
                    backgroundColor: '#666',
                    borderColor: '#AAA'
                },
                outlineColor: '#CCC',
                maskFill: 'rgba(255,255,255,0.1)',
                series: {
                    color: '#7798BF',
                    lineColor: '#A6C7ED'
                },
                xAxis: {
                    gridLineColor: '#505053'
                }
            },
            scrollbar: {
                barBackgroundColor: '#808083',
                barBorderColor: '#808083',
                buttonArrowColor: '#CCC',
                buttonBackgroundColor: '#606063',
                buttonBorderColor: '#606063',
                rifleColor: '#FFF',
                trackBackgroundColor: '#404043',
                trackBorderColor: '#404043'
            }
        };

        // Apply the theme
        Highcharts.setOptions(Highcharts.theme);
   























const cloud = document.getElementById("cloud");
const barraLateral = document.querySelector(".barra-lateral");
const spans = document.querySelectorAll("span");
const palanca = document.querySelector(".switch");
const circulo = document.querySelector(".circulo");
const menu = document.querySelector(".menu");
const main = document.querySelector("main");



menu.addEventListener("click",()=>{
    barraLateral.classList.toggle("max-barra-lateral");
    if(barraLateral.classList.contains("max-barra-lateral")){
        menu.children[0].style.display = "none";
        menu.children[1].style.display = "block";
    }
    else{
        menu.children[0].style.display = "block";
        menu.children[1].style.display = "none";
    }
    if(window.innerWidth<=500){
        barraLateral.classList.add("mini-barra-lateral");
        main.classList.add("min-main");
        spans.forEach((span)=>{
            span.classList.add("oculto");
        })
    }
});



palanca.addEventListener("click",()=>{
    let body = document.body;


    body.classList.toggle("dark-mode");
    circulo.classList.toggle("prendido");

        // Alternar entre clases dark y light
        if (body.classList.contains("dark-mode")) {
            body.classList.remove("grey");
            body.classList.add("dark");
        } else {
            body.classList.remove("dark");
            body.classList.add("grey");
        }



});

cloud.addEventListener("click",()=>{
    barraLateral.classList.toggle("mini-barra-lateral");
    main.classList.toggle("min-main");
    spans.forEach((span)=>{
        span.classList.toggle("oculto");
    });
});















//main 

const menuItems = document.querySelectorAll('.barra-lateral .navegacion a');

menuItems.forEach(item => {
    item.addEventListener('click', () => {
        // Eliminar la clase activa de todos los elementos del menú
        menuItems.forEach(menu => menu.classList.remove('active'));
        // Añadir la clase activa al elemento del menú clicado
        item.classList.add('active');
    });
});


document.getElementById('drafts').addEventListener('click', () => {
    loadContent('/drafts.html');
});
document.getElementById('spam1').addEventListener('click', () => {
    loadContent('/spam1.html');
});
document.getElementById('spam').addEventListener('click', () => {
    loadContent('/spam.html');
});




















//gauge-solid
//gauge-solid

function initHighcharts0() {
    const gaugeOptions = {
        chart: {
            type: 'solidgauge',
            backgroundColor: 'transparent' // Aquí defines el color de fondo
            
        },
        title: null,
        pane: {
            center: ['50%', '85%'],
            size: '140%',
            startAngle: -90,
            endAngle: 90,
            background: {
                backgroundColor: Highcharts.defaultOptions.legend.backgroundColor || '#000000',
                borderRadius: 5,
                innerRadius: '60%',
                outerRadius: '100%',
                shape: 'arc'
            }
        },
        exporting: {
            enabled: false
        },
        tooltip: {
            enabled: false
        },
        yAxis: {
            stops: [
                [0.1, '#55BF3B'], //#55BF3B
                [0.5, '#DDDF0D'], //DDDF0D
                [0.9, '#DF5353']  //#DF5353
            ],
            lineWidth: 0,
            tickWidth: 0,
            minorTickInterval: null,
            tickAmount: 2,
            title: {
                y: -70
            },
            labels: {
                y: 16,
                style: {
                    color: '#FFFFFF', // Cambia el color de los números del eje y
                }
            }
        },
        plotOptions: {
            solidgauge: {
                borderRadius: 3,
                dataLabels: {
                    y: 5,
                    borderWidth: 0,
                    useHTML: true,
                    style: {
                        color: '#FFFFFF' // Cambia el color de las letras de los datos
                        
                    }
                }
            }
        }
    };

    const chartSpeed = Highcharts.chart(
        'container-speed', Highcharts.merge(gaugeOptions, {
            yAxis: {
                min: 0,
                max: 30,
                title: {
                    text: 'FLUJO',
                    style: {
                        color: '#FFFFFF', // Cambia el color de las letras de los datos
                        opacity: 0.8 // Cambia la opacidad del título del eje y
                    }
                }
            },
            credits: {
                enabled: false
            },
            series: [{
                name: 'FLUJO',
                data: [32],
                dataLabels: {
                    format: '<div style="text-align:center"><span style="font-size:25px">{y}</span><br/><span style="font-size:12px;opacity:0.7">m3/h</span></div>'
                },
                tooltip: {
                    valueSuffix: ' m3/h'
                }
            }]
        })
    );

    const chartRpm = Highcharts.chart(
        'container-rpm', Highcharts.merge(gaugeOptions, {
            yAxis: {
                min: 0,
                max: 400,
                title: {
                    text: 'FLUJO ACUMULADO',
                    style: {
                        color: '#FFFFFF', // Cambia el color de las letras de los datos
                        opacity: 0.8 // Cambia la opacidad del título del eje y
                    }
                }
            },
            series: [{
                name: 'FLUJO ACUMULADO',
                data: [400],
                dataLabels: {
                    format: '<div style="text-align:center"><span style="font-size:25px">{y}</span><br/><span style="font-size:12px;opacity:0.7">m3</span></div>'
                },
                tooltip: {
                    valueSuffix: ' revolutions/min'
                }
            }]
        })
    );

    function fetchAndUpdateData() {
        fetch('/tempMachine')
            .then(response => response.text())
            .then(data => {
                const tempMachine = parseFloat(data);
                if (chartSpeed) {
                    const point = chartSpeed.series[0].points[0];
                    point.update(tempMachine);
                }
            });

        fetch('/tempMachine')
            .then(response => response.text())
            .then(data => {
                const tempMachine = parseFloat(data);
                if (chartRpm) {
                    const point = chartRpm.series[0].points[0];
                    point.update(tempMachine);
                }
            });
    }

    setInterval(fetchAndUpdateData, 2000);




    
}

//gauge-solid
//gauge-solid

















async function initHighcharts1() {
    // Inicializa la biblioteca sql.js con el archivo wasm desde cdnjs
    const SQL = await initSqlJs({ locateFile: file => `https://cdnjs.cloudflare.com/ajax/libs/sql.js/1.6.1/sql-wasm.wasm` });
    
    // Paso 1: Descargar central.db
    const centralResponse = await fetch('http://192.168.1.150/central.db');
    const centralArrayBuffer = await centralResponse.arrayBuffer();
    const centralUint8Array = new Uint8Array(centralArrayBuffer);
    const centralDb = new SQL.Database(centralUint8Array);

    // Paso 2: Obtener el último nombre de la base de datos desde la tabla db_list
    const dbListStmt = centralDb.prepare("SELECT name FROM db_list ORDER BY id DESC LIMIT 1");
    let dbName;
    if (dbListStmt.step()) {
        dbName = dbListStmt.getAsObject().name;
    }
    dbListStmt.free();

    // Paso 3: Descargar la base de datos específica usando el nombre obtenido
    const response = await fetch(`http://192.168.1.150/${dbName}`);
    const arrayBuffer = await response.arrayBuffer();
    const uInt8Array = new Uint8Array(arrayBuffer);
    const db = new SQL.Database(uInt8Array);

    // Paso 4: Consulta SQL para convertir los timestamps a hora local de Perú (GMT-5)
    const stmt = db.prepare("SELECT datetime(timestamp, '-5 hours') as local_timestamp, temperature FROM TempData");
    
    const data = [];
    while (stmt.step()) {
        const row = stmt.getAsObject();
        console.log(row.local_timestamp); // Verificación del formato del timestamp convertido
        data.push([row.local_timestamp, row.temperature]);
    }
    stmt.free();

    // Convertir los timestamps a objetos de fecha de JavaScript
    const formattedData = data.map(item => {
        const date = new Date(item[0].replace(' ', 'T') + 'Z');
        return [date.getTime(), item[1]]; // Devuelve el timestamp en milisegundos y la temperatura
    });

    // Configuración de Highcharts para mostrar los datos
    Highcharts.stockChart('container', {
        chart: {
            backgroundColor: 'rgba(255, 255, 255, 0)' // Fondo transparente
        },
        rangeSelector: {
            selected: 1,
            buttons: [
                { type: 'hour', count: 1, text: '1h' },
                { type: 'hour', count: 6, text: '6h' },
                { type: 'hour', count: 12, text: '12h' },
                { type: 'day', count: 1, text: '24h' },
                { type: 'day', count: 7, text: '7d' },
                { type: 'month', count: 1, text: '1m' }
            ]
        },
        title: {
            text: 'Historial'
        },
        series: [{
            name: 'Temp',
            data: formattedData, // Datos formateados
            tooltip: {
                valueDecimals: 2,
                valueSuffix: ' °C'
            }
        }]
    });
}










function initHighcharts2() {

// Create the chart
Highcharts.stockChart('container1', {
    chart: {
        backgroundColor: 'rgba(255, 255, 255, 0)', // Fondo transparente
        events: {
            load: function () {
                const series = this.series[0];
                let pointCount = 0; // Contador de puntos
                const maxPoints = 1800; // Número máximo de puntos a mostrar

                setInterval(function () {
                    const xhttp = new XMLHttpRequest();
                    xhttp.onreadystatechange = function() {
                        if (this.readyState == 4 && this.status == 200) {
                            const tempValue = parseFloat(this.responseText);
                            const x = (new Date()).getTime(); // current time
                            series.addPoint([x, tempValue], true, pointCount >= maxPoints);
                            pointCount++;
                        }
                    };
                    xhttp.open("GET", "/tempMachine", true);
                    xhttp.send();
                }, 1000);        //setear intervalo de actualizacion de grafica
            }
        }
    },

    accessibility: {
        enabled: false
    },

    time: {
        useUTC: false
    },

    rangeSelector: {
        buttons: [{
            count: 1,
            type: 'minute',
            text: '1M'
        }, {
            count: 5,
            type: 'minute',
            text: '5M'
        }, {
            type: 'all',
            text: 'All'
        }],
        inputEnabled: false,
        selected: 0
    },

    //title: {
    //    text: 'Temperatura'
   // },

    exporting: {
        enabled: true     //false
    },

    series: [{
        name: 'Temp',
        data: [],
        tooltip: {
           // valueDecimals: 2,
            valueSuffix: ' °C'
        }
    }]
});


}




function setTemperature() {
    const setTemp = document.getElementById('setTemp').value;
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            Swal.fire({
                icon: 'success',
                title: 'Temperatura Actualizada',
                text: 'La temperatura ha sido actualizada correctamente.'
            });
        } else if (this.readyState == 4) {
            Swal.fire({
                icon: 'error',
                title: 'Error al Actualizar Temperatura',
                text: 'Hubo un problema al actualizar la temperatura.'
            });
        }
    };
    xhttp.open("POST", "/setTemp", true);
    xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xhttp.send("value=" + setTemp);
}


























function loadContent(url) {
    const mainContent = document.getElementById('main-content');
    fetch(url)
        .then(response => response.text())
        .then(data => {
            mainContent.innerHTML = data;
            if (url.includes('drafts.html')) {
                initHighcharts0();
                initHighcharts2();
            }
            if (url.includes('spam1.html')) {
                initHighcharts1();
            }
            
         
        })
        .catch(error => console.error('Error loading content:', error));
}



//main








// Función para actualizar la temperatura seteada
function updateTempAlarm() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var tempAlarmValue = parseFloat(this.responseText);
            document.getElementById('tempAlarmRealTime').innerHTML = tempAlarmValue + ' °C';
        }
    };
    xhttp.open("GET", "/setTempAlarm", true);
    xhttp.send();
}

// Llamar a la función de actualización periódicamente
setInterval(updateTempAlarm, 2000);








// Añadir función para actualizar la temperatura actual
function updateTempMachine() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var tempValue = parseFloat(this.responseText);
            document.getElementById('currentTemp').innerHTML = tempValue + ' °C';
        }
    };
    xhttp.open("GET", "/tempMachine", true);
    xhttp.send();
}

// Llamar a la función de actualización periódicamente
setInterval(updateTempMachine, 2000);

























//main

// Cargar el contenido de main por defecto
document.addEventListener('DOMContentLoaded', () => {
    loadContent('/drafts.html');
    document.getElementById('drafts').classList.add('active');
    updateTempMachine();  //quitar
    updateTempAlarm();  //quitar
});










