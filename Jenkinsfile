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

        stage("Build service") {
            steps {
                script {
                	  sh "./release.sh"
                }
            }
        }
    }
}
