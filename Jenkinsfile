#!/usr/bin/env groovy

pipeline {
    agent any

    options {
        ansiColor("xterm")
    }

    stages {
        stage("Checkout relevant git repo") {
            steps {
                sh "sudo rm -rf * .[a-z0-9_]*"
                checkout scm
            }
        }

        stage("Build cmc/lmc service") {
            steps {
                script {
                    if (env.SERVICE_NAME == 'cmc') {
                	sh "./release_cmc.sh"
                    }
                    if (env.SERVICE_NAME == 'lmc') {
                	sh "./release_lmc.sh"
                    }
                }
            }
        }
    }
}
