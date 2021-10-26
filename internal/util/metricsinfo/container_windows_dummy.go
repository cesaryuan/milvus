// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License.

//go:build windows

package metricsinfo

import (
	"errors"
)

// IfServiceInContainer checks if the service is running inside a container
// It should be always false while under windows.
func InContainer() (bool, error) {
	return false, nil
}

// GetContainerMemLimit returns memory limit and error
func GetContainerMemLimit() (uint64, error) {
	return 0, errors.New("Not supported")
}

// GetContainerMemUsed returns memory usage and error
func GetContainerMemUsed() (uint64, error) {
	return 0, errors.New("Not supported")
}
