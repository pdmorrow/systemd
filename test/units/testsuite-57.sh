#! /bin/bash
set -xe

check_ok () {
    [ $# -eq 3 ] || return

    x="$(systemctl show --value -p "$2" "$1")"
    case "$x" in
        *$3*) return 0 ;;
        *)    return 1 ;;
    esac
}

test_recursive_template_dependency_blocking() {
    echo "Testing blocking of recursive template dependency addition"

    # A template service which will be set as a system wide service failure
    # handler via service type drop-ins. We want to test that this service
    # does not get an OnFailure dependency which is an instance of itself.
    #
    # I.e. we don't want to see failure-handler@x.service have an OnFailure
    # dependency of failure-handler@failure-handle@x.service.
    cat > /etc/systemd/system/failure-handler@.service <<EOF
[Unit]
Description=Failure handler for service "%I"

[Service]
Type=oneshot
ExecStart=/bin/false
EOF

    # Drop-in for all service units.
    mkdir /etc/systemd/system/service.d
    cat > /etc/systemd/system/service.d/10-all-services-on-failure.conf <<EOF
[Unit]
OnFailure=failure-handler@%N.service
EOF

    # Drop-in for all mount units.
    mkdir /etc/systemd/system/mount.d
    cat > /etc/systemd/system/mount.d/10-all-mounts-on-failure.conf <<EOF
[Unit]
OnFailure=failure-handler@%N.service
EOF

    # Specific drop-in for the failure-handler@.service, this unsets the
    # OnFailure dependencies added by 10-all-services-on-failure.conf
    mkdir /etc/systemd/system/failure-handler@.service.d
    cat > /etc/systemd/system/failure-handler@.service.d/99-failure-handler.conf <<EOF
[Unit]
OnFailure=
EOF

    # Also clear out OnFailure for testsuite.service itself since we don't
    # want failure-handler@.service running again if testsuite.service fails
    # for some reason.
    mkdir /etc/systemd/system/testsuite.service.d
    cat > /etc/systemd/system/testsuite.service.d/99-failure-handler.conf <<EOF
[Unit]
OnFailure=
EOF

    # This service exists so that an OnFailure dependency is added via the
    # service.d drop in.
    cat > /etc/systemd/system/failing-service.service <<EOF
[Unit]
Description=Test service which will fail

[Service]
Type=oneshot
ExecStart=/bin/false
EOF

    # Pick up the unit files.
    systemctl daemon-reload

    # Ensure the OnFailure dependency list for
    # failure-handler@failing-service.service does not include a recursive
    # dependency of failure-handler@failure-handler@failing-service.service
    check_ok failure-handler@failing-service.service OnFailure ""

    # Ensure all services except the failure handler service itself have
    # the correct OnFailure dependency set.
    check_ok failing-service.service OnFailure "failure-handler@failing-service.service"

    set +e
    systemctl start failing-service.service
    set -e

    # Check there are no recursive units lingering (i.e. we didn't create one
    # as a result of starting failure-handler@test0.service which failed).
    units=$(systemctl list-units --no-legend)
    if [[ "$units" == *"failure-handler@failure-handler"* ]]; then
        # We saw some recursive dependencies generated, fail the test.
        return 1
    fi

    # We should only have failing-service.service and
    # failure-handler@failing-service.service in the failed state. There
    # should have been no recursive services created/started
    # (failure-handler@failure-handler@failing-service.service).
    nr_failed_units="$(systemctl list-units --no-legend --no-pager | grep -c failed)"
    if [ "${nr_failed_units}" != "2"]; then
            return 1
    fi

    return 0
}

test_recursive_template_dependency_blocking

touch /testok
