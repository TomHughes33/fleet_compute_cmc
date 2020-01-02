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
                    if ($SERVICE_NAME == 'cmc') {
                	sh "./release.sh"
                    }
                    if ($SERVICE_NAME == 'lmc') {
                	sh "./release_lmc.sh"
                    }
                }
            }
        }
    }
}
